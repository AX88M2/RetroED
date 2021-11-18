#ifndef RSDK_ANIMATION_H
#define RSDK_ANIMATION_H

namespace FormatHelpers
{

class Animation
{
public:
    class Hitbox
    {
    public:
        struct HitboxInfo {
        public:
            HitboxInfo() {}

            short left   = 0;
            short top    = 0;
            short right  = 0;
            short bottom = 0;
        };

        Hitbox() {}

        HitboxInfo hitboxes[8];
    };

    class Frame
    {
    public:
        Frame() {}

        byte sheet        = 0;
        byte collisionBox = 0;
        ushort duration      = 0;
        ushort id         = 0;
        ushort sprX       = 0;
        ushort sprY       = 0;
        ushort width      = 0;
        ushort height     = 0;
        short pivotX      = 0;
        short pivotY      = 0;
        QList<Hitbox::HitboxInfo> hitboxes;
    };

    class AnimationEntry
    {
    public:
        AnimationEntry() {}

        QString name = "New Animation";
        QList<Frame> frames;
        byte loopIndex     = 0;
        short speed        = 0;
        byte rotationStyle = 0;
    };

    Animation() {}
    Animation(byte ver, QString filepath) { read(ver, filepath); }

    void read(byte ver, QString filename);
    void write(byte ver, QString filename);

    QString filePath = "";

    byte unknown[5] = { 0, 0, 0, 0, 0 };
    byte unknown2   = 0;
    byte playerType = 0;

    QList<QString> sheets;
    QList<AnimationEntry> animations;
    QList<Hitbox> hitboxes;
    QList<QString> hitboxTypes;
};

} // namespace FormatHelpers

#endif // RSDK_ANIMATION_H
