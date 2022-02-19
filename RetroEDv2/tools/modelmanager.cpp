#include "includes.hpp"
#include "ui_modelmanager.h"

ModelManager::ModelManager(QString filePath, bool usev5Format, QWidget *parent)
    : QWidget(parent), ui(new Ui::ModelManager)
{
    ui->setupUi(this);

    viewer = new ModelViewer(this);
    viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->viewerFrame->layout()->addWidget(viewer);
    viewer->show();

    connect(ui->impMDL, &QPushButton::pressed, [this] {
        QFileDialog filedialog(this, tr("Load Model Frame"), "",
                               tr(QString("OBJ Models (*.obj)").toStdString().c_str()));
        filedialog.setAcceptMode(QFileDialog::AcceptSave);
        if (filedialog.exec() == QDialog::Accepted) {
            QString selFile = filedialog.selectedFiles()[0];

            // Load Obj file :smile:
        }
    });

    connect(ui->expMDL, &QPushButton::pressed, [this] {
        QFileDialog filedialog(this, tr("Save Model Frames"), "",
                               tr(QString("OBJ Models (*.obj)").toStdString().c_str()));
        filedialog.setAcceptMode(QFileDialog::AcceptSave);
        if (filedialog.exec() == QDialog::Accepted) {
            QString selFile = filedialog.selectedFiles()[0];

            switch (mdlFormat) {
                default: break;
                case 0: modelv5.writeAsOBJ(selFile); break;
                case 1: modelv4.writeAsOBJ(selFile); break;
            }
        }
    });

    /*connect(ui->exportCurFrame, &QPushButton::pressed, [this] {
        QFileDialog filedialog(this, tr("Save Model Frame"), "",
                               tr(QString("OBJ Models (*.obj)").toStdString().c_str()));
        filedialog.setAcceptMode(QFileDialog::AcceptSave);
        if (filedialog.exec() == QDialog::Accepted) {
            QString selFile = filedialog.selectedFiles()[0];

            switch (mdlFormat) {
                default: break;
                case 0: modelv5.writeAsOBJ(selFile, -1); break;
                case 1: modelv4.writeAsOBJ(selFile, -1); break;
            }
        }
    });*/

    connect(ui->loopIndex, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int v) { viewer->loopIndex = v; });

    connect(ui->animSpeed, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v) { viewer->animSpeed = v; });

    connect(ui->useWireframe, &QCheckBox::toggled, [this](bool c) { viewer->setWireframe(c); });

    connect(ui->loadTexture, &QPushButton::clicked, [this] {
        QFileDialog filedialog(this, tr("Open Texture"), "", tr("PNG Files (*.png)"));
        filedialog.setAcceptMode(QFileDialog::AcceptOpen);
        if (filedialog.exec() == QDialog::Accepted) {
            viewer->loadTexture(filedialog.selectedFiles()[0]);
            ui->texPath->setText(filedialog.selectedFiles()[0]);
        }
    });

    ui->frameList->setCurrentRow(0);
    connect(ui->frameList, &QListWidget::currentRowChanged, [this](int r) {
        ui->animSpeed->setDisabled(r == -1);
        ui->loopIndex->setDisabled(r == -1);

        viewer->setFrame(r);
    });
    ui->frameList->setCurrentRow(-1);

    for (QWidget *w : findChildren<QWidget *>()) {
        w->installEventFilter(this);
    }

    if (QFile::exists(filePath))
        loadModel(filePath, usev5Format);
}

ModelManager::~ModelManager() { delete ui; }

bool ModelManager::event(QEvent *event)
{
    switch ((int)event->type()) {
        case RE_EVENT_NEW:
            modelv4  = RSDKv4::Model();
            modelv5  = RSDKv5::Model();
            tabTitle = "Model Manager";
            setupUI();
            return true;

        case RE_EVENT_OPEN: {
            QString filters = { "RSDKv5 Model Files (*.bin);;RSDKv4 Model Files (*.bin)" };

            QFileDialog filedialog(this, tr("Open RSDK Model"), "", tr(filters.toStdString().c_str()));
            filedialog.setAcceptMode(QFileDialog::AcceptOpen);
            if (filedialog.exec() == QDialog::Accepted) {
                loadModel(filedialog.selectedFiles()[0],
                          filedialog.selectedNameFilter() == "RSDKv5 Model Files (*.bin)");
                return true;
            }
            break;
        }

        case RE_EVENT_SAVE:
            if (saveModel())
                return true;
            break;

        case RE_EVENT_SAVE_AS:
            if (saveModel(true))
                return true;
            break;
    }

    return QWidget::event(event);
}

bool ModelManager::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
        QApplication::sendEvent(this, event);
        return false;
    }

    if (object != viewer)
        return false;

    switch ((int)event->type()) {
        default: break;
    }

    return false;
}

void ModelManager::setupUI()
{
    ui->frameList->blockSignals(true);

    ui->frameList->clear();
    switch (mdlFormat) {
        default: break;
        case 0: {
            for (int f = 0; f < modelv5.frames.count(); ++f)
                ui->frameList->addItem("Frame " + QString::number(f));

            ui->animSpeed->blockSignals(true);
            ui->animSpeed->setValue(0);
            viewer->animSpeed = 0;
            ui->animSpeed->blockSignals(false);

            ui->loopIndex->blockSignals(true);
            ui->loopIndex->setValue(0);
            viewer->loopIndex = 0;
            ui->loopIndex->blockSignals(false);

            ui->frameList->setCurrentRow(-1);

            viewer->setModel(modelv5);
            break;
        }

        case 1: {
            for (int f = 0; f < modelv4.frames.count(); ++f)
                ui->frameList->addItem("Frame " + QString::number(f));

            ui->animSpeed->blockSignals(true);
            ui->animSpeed->setValue(0);
            viewer->animSpeed = 0;
            ui->animSpeed->blockSignals(false);

            ui->loopIndex->blockSignals(true);
            ui->loopIndex->setValue(0);
            viewer->loopIndex = 0;
            ui->loopIndex->blockSignals(false);

            ui->frameList->setCurrentRow(-1);

            viewer->setModel(modelv4);
            break;
        }
    }

    ui->frameList->blockSignals(false);
}

void ModelManager::loadModel(QString filePath, bool usev5Format)
{
    setStatus("Loading model...", true);
    if (usev5Format) {
        mdlFormat = 0;
        modelv5.read(filePath);
        tabTitle = Utils::getFilenameAndFolder(modelv5.filePath);
    }
    else {
        mdlFormat = 1;
        modelv4.read(filePath);
        tabTitle = Utils::getFilenameAndFolder(modelv4.filePath);
    }
    setStatus("Loaded model " + tabTitle);

    appConfig.addRecentFile(mdlFormat == 0 ? ENGINE_v5 : ENGINE_v4, TOOL_MODELMANAGER, filePath,
                            QList<QString>{});
    setupUI();
}

bool ModelManager::saveModel(bool forceSaveAs)
{
    switch (mdlFormat) {
        default: break;
        case 0: {
            if (forceSaveAs || modelv5.filePath.isEmpty()) {
                QFileDialog filedialog(this, tr("Save RSDK Model"), "",
                                       tr("RSDKv5 Model Files (*.bin)"));
                filedialog.setAcceptMode(QFileDialog::AcceptSave);
                if (filedialog.exec() == QDialog::Accepted) {
                    setStatus("Saving model...", true);

                    QString filepath = filedialog.selectedFiles()[0];

                    modelv5.write(filepath);
                    setStatus("Saved model to " + filepath);

                    appConfig.addRecentFile(mdlFormat == 0 ? ENGINE_v5 : ENGINE_v4, TOOL_MODELMANAGER,
                                            filepath, QList<QString>{});
                    updateTitle(false);
                    return true;
                }
            }
            else {
                setStatus("Saving model...", true);

                modelv5.write();
                setStatus("Saved model to " + modelv5.filePath);

                appConfig.addRecentFile(mdlFormat == 0 ? ENGINE_v5 : ENGINE_v4, TOOL_MODELMANAGER,
                                        modelv5.filePath, QList<QString>{});
                updateTitle(false);
                return true;
            }
            break;
        }

        case 1: {
            if (forceSaveAs || modelv4.filePath.isEmpty()) {
                QFileDialog filedialog(this, tr("Save RSDK Model"), "",
                                       tr("RSDKv4 Model Files (*.bin)"));
                filedialog.setAcceptMode(QFileDialog::AcceptSave);
                if (filedialog.exec() == QDialog::Accepted) {
                    setStatus("Saving model...", true);

                    QString filepath = filedialog.selectedFiles()[0];

                    modelv4.write(filepath);
                    setStatus("Saved model to " + filepath);

                    appConfig.addRecentFile(mdlFormat == 0 ? ENGINE_v5 : ENGINE_v4, TOOL_MODELMANAGER,
                                            filepath, QList<QString>{});
                    updateTitle(false);
                    return true;
                }
            }
            else {
                setStatus("Saving model...", true);

                modelv4.write();

                appConfig.addRecentFile(mdlFormat == 0 ? ENGINE_v5 : ENGINE_v4, TOOL_MODELMANAGER,
                                        modelv4.filePath, QList<QString>{});
                updateTitle(false);
                setStatus("Saved model to " + modelv5.filePath);

                return true;
            }
            break;
        }
    }

    return false;
}

#include "moc_modelmanager.cpp"
