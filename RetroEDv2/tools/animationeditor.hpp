#pragma once

#include <QWidget>

namespace Ui
{
class AnimationEditor;
}
class AnimationEditor : public QWidget
{
    Q_OBJECT
public:
    class ActionState
    {
    public:
        QString name = "Action";

        FormatHelpers::Animation animFile;
    };

    explicit AnimationEditor(QString filepath = "", byte type = 0xFF, QWidget *parent = nullptr);
    ~AnimationEditor();

    void SetupUI(bool setFrame = false, bool setRow = false);
    void UpdateView();
    void ProcessAnimation();

    bool event(QEvent *event) override;

    void SetupFormatConversion(int aniVer, int prevAniVer);

    inline int FrameCount()
    {
        if (currentAnim < animFile.animations.count()) {
            return animFile.animations[currentAnim].frames.count();
        }
        return 0;
    }

    inline void UpdateTitle(bool modified)
    {
        this->modified = modified;
        if (modified)
            emit TitleChanged(tabTitle + " *", tabPath);
        else
            emit TitleChanged(tabTitle, tabPath);
    }

signals:
    void TitleChanged(QString title, QString tabPath);

private:
    Ui::AnimationEditor *ui;

    FormatHelpers::Animation animFile;
    QList<bool> hitboxVisible;
    int aniType = ENGINE_v5;

    ushort currentAnim    = -1;
    ushort currentFrame   = -1;
    ushort currentHitbox  = -1;
    byte currentSubHitbox = -1;

    inline void StartAnim()
    {
        playingAnim  = true;
        animFinished = false;
        animTimer    = 0;
        prevFrame    = 0;
        updateTimer->start(1000.0f / 60.0f);
    }

    inline void stopAnim()
    {
        playingAnim  = false;
        animFinished = false;
        animTimer    = 0;
        prevFrame    = 0;

        updateTimer->stop();
    }
    void SetFramePreview(bool update = false);
    void SetupFrameList(QList<FormatHelpers::Animation::Frame> &frames);

    bool showTransparentClr = true;
    QColor bgColor          = Qt::transparent;
    float zoom              = 1.0;

    bool playingAnim  = false;
    int animTimer     = 0;
    bool animFinished = false;
    int prevFrame     = 0;

    bool mouseDownL = false;
    bool mouseDownM = false;
    bool mouseDownR = false;

    bool savedPivToggle = false;
    QColor savedColor = QColor(0xFFFFFFFF);

    Vector2<int> offset = Vector2<int>(0, 0);

    QPoint reference;
    Vector2<float> mousePos = Vector2<float>(0.0f, 0.0f);

    QTimer *updateTimer            = nullptr;
    QStandardItemModel *frameModel = nullptr;

    bool linesInFront = true;

    void LoadSheet(QString filepath, int index, bool addSource = true);
    void RemoveSheet(int index, bool removeSource = true);
    void MoveSheet(int from, int to);

    QString GetBaseDir();

    void LoadAnim(QString filepath, int aniType);
    void RotateHitboxes();

    QVector<QImage> sheets;

    QPixmap missingImg;

    void UndoAction();
    void RedoAction();
    void ResetAction();
    void DoAction(QString name = "Action", bool setModified = true);
    void ClearActions();

    void CopyAnimFile(FormatHelpers::Animation &src, FormatHelpers::Animation &dst);

    QList<ActionState> actions;
    int actionIndex = 0;

    bool modified    = false;
    QString tabTitle = "Animation Editor";
    QString tabPath  = "";
};
