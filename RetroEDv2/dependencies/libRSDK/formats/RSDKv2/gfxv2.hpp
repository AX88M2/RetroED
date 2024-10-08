#pragma once

namespace RSDKv2
{

class GFX
{
public:
    GFX() {}
    GFX(QString filename) { read(filename); }
    GFX(Reader &reader) { read(reader); }

    inline void read(QString filename)
    {
        Reader reader(filename);
        read(reader);
    }
    void read(Reader &reader, bool dcGFX = false);

    inline void write(QString filename)
    {
        if (filename == "")
            filename = filePath;
        if (filename == "")
            return;
        Writer writer(filename);
        write(writer);
    }
    void write(Writer &writer, bool dcGFX = false);

    void importImage(QImage image);
    void importImage(FormatHelpers::Gif image);

    QImage exportImage();
    FormatHelpers::Gif exportGif();

    Color palette[255];
    ushort width  = 16;
    ushort height = 16;
    QByteArray pixels;

    QString filePath = "";
};

} // namespace RSDKv2


