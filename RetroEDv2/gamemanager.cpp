#include "includes.hpp"
#include "ui_gamemanager.h"

#include "gamemanager.hpp"

GameManager::GameManager(QWidget *parent) : QDialog(parent), ui(new Ui::GameManager)
{
    ui->setupUi(this);

    // remove question mark from the title bar & disable resizing
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(QSize(width(), height()));

    QPushButton *setExeLocation[] = { ui->setExeLocV5, ui->setExeLocV4, ui->setExeLocV3,
                                      ui->setExeLocV2, ui->setExeLocV1 };
    QPushButton *setDataLocation[] = { ui->setDataLocV5, ui->setDataLocV4, ui->setDataLocV3,
                                       ui->setDataLocV2, ui->setDataLocV1 };
    QPushButton *setGameLogicLocation[] = { ui->setGameLogicLocV5, nullptr, nullptr,
                                       nullptr, nullptr };
    QLineEdit *exeLocation[] = { ui->exeLocV5, ui->exeLocV4, ui->exeLocV3, ui->exeLocV2, ui->exeLocV1 };
    QLineEdit *gameLogicLocation[] = { ui->gameLogicLocV5, nullptr, nullptr, nullptr, nullptr };
    QLineEdit *dataLocation[] = { ui->dataLocV5, ui->dataLocV4, ui->dataLocV3, ui->dataLocV2, ui->dataLocV1 };

    ui->versionToolbox->setCurrentIndex(0);
    for (int v = 0; v <= ENGINE_v1; ++v) {
        exeLocation[v]->setText(appConfig.gameManager[v].exePath);
        if(v == ENGINE_v5){
            gameLogicLocation[v]->setText(appConfig.gameLogicManager[v].gameLogicPath);
        }
        dataLocation[v]->setText(appConfig.baseDataManager[v].dataPath);

        disconnect(setExeLocation[v], nullptr, nullptr, nullptr);
        if(v == ENGINE_v5){
            disconnect(setGameLogicLocation[v], nullptr, nullptr, nullptr);
        }
        disconnect(setDataLocation[v], nullptr, nullptr, nullptr);

        connect(setExeLocation[v], &QPushButton::clicked, [this, exeLocation, v] {
#if defined(Q_OS_WIN)
            QFileDialog filedialog(this, tr("Open Executable"), "",
                                   "Windows Executables (*.exe);;All Files (*)");
#elif defined(Q_OS_MACOS)
            QFileDialog filedialog(this, tr("Open Executable"), "",
                                   "Mac OS Executables (*.app);;All Files (*)");
#else
            QFileDialog filedialog(this, tr("Open Executable"), "",
                                   "All Files (*)");
#endif
            filedialog.setAcceptMode(QFileDialog::AcceptOpen);
            if (filedialog.exec() == QDialog::Accepted) {
                QString exePath = filedialog.selectedFiles()[0];
                exeLocation[v]->setText(exePath);

                appConfig.gameManager[v].exePath = exePath;
                appConfig.write();
            }
        });

        if(v == ENGINE_v5){
            connect(setGameLogicLocation[v], &QPushButton::clicked, [this, gameLogicLocation, v] {
#if defined(Q_OS_WIN)
                QFileDialog filedialog(this, tr("Open Gamelogic"), "",
                                       "Windows Executables (*.dll);;All Files (*)");
#elif defined(Q_OS_MACOS)
                QFileDialog filedialog(this, tr("Open Gamelogic"), "",
                                       "All Files (*)");
#else
                QFileDialog filedialog(this, tr("Open Gamelogic"), "",
                                       "All Files (*)");
#endif
                filedialog.setAcceptMode(QFileDialog::AcceptOpen);
                if (filedialog.exec() == QDialog::Accepted) {
                    QString gameLogicPath = filedialog.selectedFiles()[0];
                    gameLogicLocation[v]->setText(gameLogicPath);

                    appConfig.gameLogicManager[v].gameLogicPath = gameLogicPath;
                    appConfig.write();
                }
            });
        }

        connect(setDataLocation[v], &QPushButton::clicked, [this, dataLocation, v] {
            QFileDialog filedialog(this, tr("Open Directory"), "", "");
            filedialog.setFileMode(QFileDialog::FileMode::Directory);
            filedialog.setAcceptMode(QFileDialog::AcceptOpen);
            if (filedialog.exec() == QDialog::Accepted) {
                QString dataPath = filedialog.selectedFiles()[0];
                dataLocation[v]->setText(dataPath);
                appConfig.baseDataManager[v].dataPath = dataPath;
                appConfig.write();
            }
        });

        connect(dataLocation[v], &QLineEdit::textEdited, [dataLocation, v](QString s){
            dataLocation[v]->setText(s);
            appConfig.baseDataManager[v].dataPath = s;
            appConfig.write();
        });
        if(v == ENGINE_v5){
            connect(gameLogicLocation[v], &QLineEdit::textEdited, [gameLogicLocation, v](QString s){
                gameLogicLocation[v]->setText(s);
                appConfig.gameLogicManager[v].gameLogicPath = s;
                appConfig.write();
            });
        }
    }
}

GameManager::~GameManager() { delete ui; }
