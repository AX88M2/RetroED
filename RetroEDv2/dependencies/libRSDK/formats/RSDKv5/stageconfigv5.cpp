#include "libRSDK.hpp"

#include "stageconfigv5.hpp"

void RSDKv5::StageConfig::read(Reader &reader)
{
    filePath = reader.filePath;

    if (!reader.matchesSignature(signature, 4))
        return;

    loadGlobalObjects = reader.read<bool>();

    byte objCnt = reader.read<byte>();
    objects.clear();
    for (int i = 0; i < objCnt; ++i) objects.append(reader.readString());

    for (int i = 0; i < 8; ++i) palettes[i].read(reader);

    byte sfxCnt = reader.read<byte>();
    soundFX.clear();
    for (int i = 0; i < sfxCnt; ++i) soundFX.append(WAVConfiguration(reader));
}

void RSDKv5::StageConfig::write(Writer &writer)
{
    filePath = writer.filePath;
    writer.write(signature, 4);

    writer.write(loadGlobalObjects);

    writer.write((byte)objects.count());
    for (int i = 0; i < (byte)objects.count(); ++i) writer.write(objects[i]);

    for (int i = 0; i < 8; ++i) palettes[i].write(writer, true);

    writer.write((byte)soundFX.count());
    for (int i = 0; i < (byte)soundFX.count(); ++i) soundFX[i].write(writer);

    writer.flush();
}
