#pragma once

#include <QWidget>
#include "tools/utils/modelviewer.hpp"

namespace Ui
{
class ModelManager;
}

class ModelManager : public QWidget
{
    Q_OBJECT

public:
    explicit ModelManager(QString filePath = "", bool usev5Format = true, QWidget *parent = nullptr);
    ~ModelManager();

    void SetupUI(bool initialSetup = true);
    ModelViewer *viewer = nullptr;

    void LoadModel(QString filePath, bool usev5Format);
    bool SaveModel(bool forceSaveAs = false);

    inline void UpdateTitle(bool modified)
    {
        this->modified = modified;
        if (modified)
            emit TitleChanged(tabTitle + " *");
        else
            emit TitleChanged(tabTitle);
    }

signals:
    void TitleChanged(QString title);

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::ModelManager *ui;

    color_widgets::ColorPreview *mdlClrSel = nullptr;

    void StartAnim();
    void StopAnim();

    void ProcessAnimation();

    ushort currentFrame = -1;
    bool playingAnim    = false;
    bool animFinished   = false;
    int prevFrame       = 0;
    QTimer *updateTimer = nullptr;

    QPointF lastMousePos;
    bool ctrlDownL  = false;
    bool mouseDownL = false;
    bool mouseDownM = false;
    bool mouseDownR = false;

    bool modified    = false;
    QString tabTitle = "Model Manager";
};
