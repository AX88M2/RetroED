#include "libRSDK.hpp"

#include "scenev3.hpp"

void RSDKv3::Scene::read(Reader &reader)
{
    filePath = reader.filePath;

    title = reader.readString();

    for (int i = 0; i < 4; ++i) activeLayer[i] = reader.read<byte>();
    midpoint = reader.read<byte>();

    // Map width in 128 pixel units
    // In RSDKv3, it's one byte long
    width  = reader.read<byte>();
    height = reader.read<byte>();

    layout.reserve(height);
    for (int y = 0; y < height; ++y) {
        layout.append(QList<ushort>());
        layout[y].reserve(width);
        for (int x = 0; x < width; ++x) {
            // 128x128 Block number is 16-bit
            // Big-Endian in RSDKv2 and RSDKv3
            byte b0 = 0, b1 = 0;
            b0 = reader.read<byte>();
            b1 = reader.read<byte>();
            layout[y].append((ushort)(b1 + (b0 << 8)));
        }
    }

    // Read number of object types, Only RSDKv2 and RSDKv3 support this feature
    int objTypeCount = reader.read<byte>();
    for (int n = 0; n < objTypeCount; ++n) typeNames.append(reader.readString());
    // Read object data

    int objCount = 0;

    // 2 bytes, big-endian, unsigned
    objCount = reader.read<byte>() << 8;
    objCount |= reader.read<byte>();

    for (int n = 0; n < objCount; ++n) entities.append(Entity(reader, n));
}

void RSDKv3::Scene::write(Writer &writer)
{
    filePath = writer.filePath;

    // Write zone name
    writer.write(title);

    // Write the five "display" bytes
    for (int i = 0; i < 4; ++i) writer.write(activeLayer[i]);
    writer.write(midpoint);

    // Write width and height
    writer.write((byte)width);
    writer.write((byte)height);

    // Write tile map

    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            writer.write((byte)(layout[h][w] >> 8));
            writer.write((byte)(layout[h][w] & 0xff));
        }
    }

    writer.write((byte)(typeNames.count()));

    // Write object type names
    // Ignore first object type "Type zero", it is not stored.
    for (int n = 0; n < typeNames.count(); ++n) writer.write(typeNames[n]);

    // Write number of objects
    writer.write((byte)(entities.count() >> 8));
    writer.write((byte)(entities.count() & 0xFF));

    std::sort(entities.begin(), entities.end(),
              [](const Entity &a, const Entity &b) -> bool { return a.slotID < b.slotID; });

    // Write object data
    for (Entity &obj : entities) obj.write(writer);
    writer.flush();
}
