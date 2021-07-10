#include "includes.hpp"

static QVector3D rectVertices[] = {
    QVector3D(-0.5f, -0.5f, -0.5f), QVector3D(0.5f, -0.5f, -0.5f), QVector3D(0.5f, 0.5f, -0.5f),
    QVector3D(0.5f, 0.5f, -0.5f),   QVector3D(-0.5f, 0.5f, -0.5f), QVector3D(-0.5f, -0.5f, -0.5f),
};

static QVector2D rectTexCoords[] = {
    QVector2D(0.0f, 0.0f), QVector2D(1.0f, 0.0f), QVector2D(1.0f, 1.0f),
    QVector2D(1.0f, 1.0f), QVector2D(0.0f, 1.0f), QVector2D(0.0f, 0.0f),
};

SceneViewer::SceneViewer(QWidget *parent)
{
    setMouseTracking(true);

    this->setFocusPolicy(Qt::WheelFocus);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&SceneViewer::updateScene));
    timer->start(1000.0f / 60.0f);
}

SceneViewer::~SceneViewer()
{
    unloadScene();

    screenVAO.destroy();
    rectVAO.destroy();
}

void SceneViewer::loadScene(QString path, byte ver)
{
    // unloading
    unloadScene();

    // loading
    QString pth      = path;
    QString basePath = pth.replace(QFileInfo(pth).fileName(), "");

    scene.read(ver, path);

    if (ver != ENGINE_v1) {
        background.read(ver, basePath + "Backgrounds.bin");
        chunkset.read(ver, basePath + "128x128Tiles.bin");
        tileconfig.read(basePath + "CollisionMasks.bin");
        stageConfig.read(ver, basePath + "StageConfig.bin");
    }
    else {
        background.read(ver, basePath + "ZoneBG.map");
        chunkset.read(ver, basePath + "Zone.til");
        tileconfig.read(basePath + "Zone.tcf");
        stageConfig.read(ver, basePath + "Zone.zcf");
    }

    // Always have 8 layers, even if some have w/h of 0
    for (int l = background.layers.count(); l < 8; ++l)
        background.layers.append(FormatHelpers::Background::Layer());

    if (ver != ENGINE_v1) {
        scene.objectTypeNames.clear();

        if (stageConfig.loadGlobalScripts) {
            if (ver == ENGINE_v2)
                scene.objectTypeNames.append("Player");

            for (FormatHelpers::Gameconfig::ObjectInfo &obj : gameConfig.objects) {
                scene.objectTypeNames.append(obj.m_name);
            }
        }

        for (FormatHelpers::Stageconfig::ObjectInfo &obj : stageConfig.objects) {
            scene.objectTypeNames.append(obj.m_name);
        }
    }
    else {
        scene.objectTypeNames.clear();

        // TODO: globals
        QList<QString> globals = {
            "Ring",              // 1
            "Dropped Ring",      // 2
            "Ring Sparkle",      // 3
            "Monitor",           // 4
            "Broken Monitor",    // 5
            "Yellow Spring",     // 6
            "Red Spring",        // 7
            "Spikes",            // 8
            "StarPost",          // 9
            "PlaneSwitch A",     // 10
            "PlaneSwitch B",     // 11
            "Unknown (ID 12)",   // 12
            "Unknown (ID 13)",   // 13
            "ForceSpin R",       // 14
            "ForceSpin L",       // 15
            "Unknown (ID 16)",   // 16
            "Unknown (ID 17)",   // 17
            "SignPost",          // 18
            "Egg Prison",        // 19
            "Small Explosion",   // 20
            "Large Explosion",   // 21
            "Egg Prison Debris", // 22
            "Animal",            // 23
            "Ring",              // 24
            "Ring",              // 25
            "Big Ring",          // 26
            "Water Splash",      // 27
            "Bubbler",           // 28
            "Small Air Bubble",  // 29
            "Smoke Puff",        // 30
        };

        for (QString &obj : globals) {
            scene.objectTypeNames.append(obj);
        }

        for (FormatHelpers::Stageconfig::ObjectInfo &obj : stageConfig.objects) {
            scene.objectTypeNames.append(obj.m_name);
        }
    }

    if (ver == ENGINE_v1) {
        if (QFile::exists(basePath + "Zone.gfx")) {
            // setup tileset texture from png
            RSDKv1::GFX gfx(basePath + "Zone.gfx");
            QImage tileset   = gfx.exportImage();
            m_tilesetTexture = createTexture(tileset);
            for (int i = 0; i < 0x400; ++i) {
                int tx         = ((i % (tileset.width() / 0x10)) * 0x10);
                int ty         = ((i / (tileset.width() / 0x10)) * 0x10);
                QImage tileTex = tileset.copy(tx, ty, 0x10, 0x10);

                tiles.append(tileTex);
            }

            for (FormatHelpers::Chunks::Chunk &c : chunkset.chunks) {
                QImage img = c.getImage(tiles);
                chunks.append(img);
            }
        }
    }
    else {
        if (QFile::exists(basePath + "16x16Tiles.gif")) {
            // setup tileset texture from png
            QImage tileset(basePath + "16x16Tiles.gif");
            m_tilesetTexture = createTexture(tileset);
            for (int i = 0; i < 0x400; ++i) {
                int tx         = ((i % (tileset.width() / 0x10)) * 0x10);
                int ty         = ((i / (tileset.width() / 0x10)) * 0x10);
                QImage tileTex = tileset.copy(tx, ty, 0x10, 0x10);

                tiles.append(tileTex);
            }

            for (FormatHelpers::Chunks::Chunk &c : chunkset.chunks) {
                QImage img = c.getImage(tiles);
                chunks.append(img);
            }
        }
    }

    // objects
    objectSprites.clear();
    {
        TextureInfo tex;
        tex.name       = ":/icons/missing.png";
        missingObj     = QImage(tex.name);
        tex.texturePtr = createTexture(missingObj);
        objectSprites.append(tex);
    }

    m_rsPlayerSprite = createTexture(QImage(":/icons/player_v1.png"));
}

void SceneViewer::updateScene()
{
    this->repaint();

    if (statusLabel) {
        int mx = (int)((m_mousePos.x * invZoom()) + cam.pos.x);
        int my = (int)((m_mousePos.y * invZoom()) + cam.pos.y);
        statusLabel->setText(
            QString("Zoom: %1%, Mouse Position: (%2, %3), Chunk Position: (%4, %5), Selected Chunk: "
                    "%6, Selected Layer: %7 (%8), Selected Object: %9")
                .arg(zoom * 100)
                .arg(mx)
                .arg(my)
                .arg((int)mx / 0x80)
                .arg((int)my / 0x80)
                .arg(selectedChunk)
                .arg(selectedLayer)
                .arg(selectedLayer >= 0 && 9 ? selectedLayer == 0
                                                   ? "Foreground"
                                                   : "Background " + QString::number(selectedLayer)
                                             : "")
                .arg(selectedObject >= 0 && selectedObject < scene.objects.count()
                         ? scene.objectTypeNames[selectedObject]
                         : ""));
    }
}

void SceneViewer::drawScene()
{
    if (!m_tilesetTexture)
        return;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glBlendEquation(GL_FUNC_ADD);

    // pre-render
    if ((cam.pos.x * zoom) < 0 * zoom)
        cam.pos.x = (0 * zoom);

    if ((cam.pos.y * zoom) < 0 * zoom)
        cam.pos.y = (0 * zoom);

    if ((cam.pos.x * zoom) + storedW > (scene.width * 0x80) * zoom)
        cam.pos.x = ((scene.width * 0x80) - (storedW * invZoom()));

    if ((cam.pos.y * zoom) + storedH > (scene.height * 0x80) * zoom)
        cam.pos.y = ((scene.height * 0x80) - (storedH * invZoom()));

    // draw bg colours
    primitiveShader.use();
    primitiveShader.setValue("colour", QVector4D(m_altBGColour.r / 255.0f, m_altBGColour.g / 255.0f,
                                                 m_altBGColour.b / 255.0f, 1.0f));
    primitiveShader.setValue("projection", getProjectionMatrix());
    primitiveShader.setValue("view", QMatrix4x4());
    rectVAO.bind();

    int bgOffsetY = 0x80;
    bgOffsetY -= (int)cam.pos.y % 0x200;
    for (int y = bgOffsetY; y < (storedH + 0x80) * (zoom < 1.0f ? invZoom() : 1.0f); y += 0x100) {
        int bgOffsetX = (((y - bgOffsetY) % 0x200 == 0) ? 0x100 : 0x00);
        bgOffsetX += 0x80;
        bgOffsetX -= (int)cam.pos.x % 0x200;
        for (int x = bgOffsetX; x < (storedW + 0x80) * (zoom < 1.0f ? invZoom() : 1.0f); x += 0x200) {
            QMatrix4x4 matModel;
            matModel.scale(0x100 * zoom, 0x100 * zoom, 1.0f);
            matModel.translate(x / 256.0f, y / 256.0f, -15.0f);
            primitiveShader.setValue("model", matModel);

            f->glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    spriteShader.use();
    spriteShader.setValue("useAlpha", false);
    spriteShader.setValue("alpha", 1.0f);
    spriteShader.setValue("transparentColour", (gameType != ENGINE_v1 ? QVector3D(1.0f, 0.0f, 1.0f)
                                                                      : QVector3D(0.0f, 0.0f, 0.0f)));

    QMatrix4x4 matWorld;
    QMatrix4x4 matView;
    spriteShader.setValue("projection", matWorld = getProjectionMatrix());
    spriteShader.setValue("view", matView = QMatrix4x4());
    f->glActiveTexture(GL_TEXTURE0);

    rectVAO.bind();

    int prevChunk = -1;
    Vector3<float> camOffset(0.0f, 0.0f, 0.0f);

    QVector4D pixelSolidityClrs[5] = { QVector4D(1.0f, 1.0f, 1.0f, 1.0f),
                                       QVector4D(1.0f, 1.0f, 0.0f, 1.0f),
                                       QVector4D(1.0f, 0.0f, 0.0f, 1.0f),
                                       QVector4D(0.0f, 0.0f, 0.0f, 0.0f),
                                       QVector4D(0.0f, 0.0f, 1.0f, 1.0f) };
    bool showCLayers[2]            = { showPlaneA, showPlaneB };

    for (int l = 8; l >= 0; --l) {
        // TILE LAYERS
        QList<QList<ushort>> layout = scene.layout;
        int width                   = scene.width;
        int height                  = scene.height;

        if (l > 0) {
            layout = background.layers[l - 1].layout;
            width  = background.layers[l - 1].width;
            height = background.layers[l - 1].height;
        }

        spriteShader.use();
        spriteShader.setValue("useAlpha", false);
        spriteShader.setValue("alpha", 1.0f);
        spriteShader.setValue(
            "transparentColour",
            (gameType != ENGINE_v1 ? QVector3D(1.0f, 0.0f, 1.0f) : QVector3D(0.0f, 0.0f, 0.0f)));
        rectVAO.bind();
        // manage properties
        camOffset = Vector3<float>(0.0f, 0.0f, 0.0f);

        if (selectedLayer >= 0) {
            spriteShader.setValue("useAlpha", true);
            if (selectedLayer == l) {
                spriteShader.setValue("alpha", 1.0f);
            }
            else {
                spriteShader.setValue("alpha", 0.5f);
            }
        }

        // draw
        m_tilesetTexture->bind();
        spriteShader.setValue("flipX", false);
        spriteShader.setValue("flipY", false);
        spriteShader.setValue("useAlpha", false);
        spriteShader.setValue("alpha", 1.0f);

        QVector3D *vertsPtr  = new QVector3D[height * width * 0x80 * 6];
        QVector2D *tVertsPtr = new QVector2D[height * width * 0x80 * 6];
        int vertCnt          = 0;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                ushort chunkID = layout[y][x];
                if (chunkID != 0x0) {
                    for (int ty = 0; ty < 8; ++ty) {
                        for (int tx = 0; tx < 8; ++tx) {
                            FormatHelpers::Chunks::Tile &tile = chunkset.chunks[chunkID].tiles[ty][tx];

                            float xpos = (x * 0x80) + (tx * 0x10) - (cam.pos.x + camOffset.x);
                            float ypos = (y * 0x80) + (ty * 0x10) - (cam.pos.y + camOffset.y);
                            float zpos = selectedLayer == l ? 8.5 : (8 - l);
                            if (tile.visualPlane == 1)
                                zpos += 0.1; // high plane

                            Rect<int> check = Rect<int>();
                            check.x         = (int)((xpos + 0x10) * zoom);
                            check.y         = (int)((ypos + 0x10) * zoom);
                            check.w         = (int)((xpos - (0x10 / 2)) * zoom);
                            check.h         = (int)((ypos - (0x10 / 2)) * zoom);

                            if (check.x < 0 || check.y < 0 || check.w >= storedW
                                || check.h >= storedH) {
                                continue;
                            }

                            vertsPtr[vertCnt + 0].setX(0.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 0].setY(0.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 0].setZ(zpos);

                            vertsPtr[vertCnt + 1].setX(1.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 1].setY(0.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 1].setZ(zpos);

                            vertsPtr[vertCnt + 2].setX(1.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 2].setY(1.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 2].setZ(zpos);

                            vertsPtr[vertCnt + 3].setX(1.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 3].setY(1.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 3].setZ(zpos);

                            vertsPtr[vertCnt + 4].setX(0.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 4].setY(1.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 4].setZ(zpos);

                            vertsPtr[vertCnt + 5].setX(0.0f + (xpos / 0x10));
                            vertsPtr[vertCnt + 5].setY(0.0f + (ypos / 0x10));
                            vertsPtr[vertCnt + 5].setZ(zpos);

                            getTileVerts(tVertsPtr, vertCnt, tile.tileIndex * 0x10, tile.direction);
                            vertCnt += 6;
                        }
                    }
                }
            }
        }

        // Draw Tiles
        {
            QOpenGLVertexArrayObject vao;
            vao.create();
            vao.bind();

            QOpenGLBuffer vVBO2D;
            vVBO2D.create();
            vVBO2D.setUsagePattern(QOpenGLBuffer::StaticDraw);
            vVBO2D.bind();
            vVBO2D.allocate(vertsPtr, vertCnt * sizeof(QVector3D));
            spriteShader.enableAttributeArray(0);
            spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

            QOpenGLBuffer tVBO2D;
            tVBO2D.create();
            tVBO2D.setUsagePattern(QOpenGLBuffer::StaticDraw);
            tVBO2D.bind();
            tVBO2D.allocate(tVertsPtr, vertCnt * sizeof(QVector2D));
            spriteShader.enableAttributeArray(1);
            spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

            QMatrix4x4 matModel;
            matModel.scale(0x10 * zoom, 0x10 * zoom, 1.0f);
            spriteShader.setValue("model", matModel);

            f->glDrawArrays(GL_TRIANGLES, 0, vertCnt);

            vao.release();
            tVBO2D.release();
            vVBO2D.release();

            delete[] vertsPtr;
            delete[] tVertsPtr;
        }

        // Collision Previews
        for (int c = 0; c < 2; ++c) {
            if (showCLayers[c]) {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        ushort chunkID = layout[y][x];
                        if (chunkID != 0x0) {
                            for (int ty = 0; ty < 8; ++ty) {
                                for (int tx = 0; tx < 8; ++tx) {
                                    FormatHelpers::Chunks::Tile &tile =
                                        chunkset.chunks[chunkID].tiles[ty][tx];

                                    float xpos = (x * 0x80) + (tx * 0x10) - cam.pos.x;
                                    float ypos = (y * 0x80) + (ty * 0x10) - cam.pos.y;

                                    Rect<int> check = Rect<int>();
                                    check.x         = (int)((xpos + 0x10) * zoom);
                                    check.y         = (int)((ypos + 0x10) * zoom);
                                    check.w         = (int)(xpos * zoom);
                                    check.h         = (int)(ypos * zoom);

                                    if (check.x < 0 || check.y < 0 || check.w >= storedW
                                        || check.h >= storedH) {
                                        continue;
                                    }

                                    float yStore = ypos;
                                    // draw pixel collision
                                    byte solidity = 0;
                                    RSDKv4::Tileconfig::CollisionMask &cmask =
                                        tileconfig.collisionPaths[c][tile.tileIndex];

                                    solidity = !c ? tile.solidityA : tile.solidityB;

                                    if (solidity == 3)
                                        continue;

                                    for (byte cx = 0; cx < 16; ++cx) {
                                        int hm = cx;
                                        if (Utils::getBit(tile.direction, 0))
                                            hm = 15 - cx;

                                        if (!cmask.collision[hm].solid)
                                            continue;

                                        byte cy = cmask.collision[hm].height;
                                        byte ch = 16 - cy;
                                        if (Utils::getBit(tile.direction, 1) && !cmask.flipY) {
                                            cy = 0;
                                            ch = 16 - cmask.collision[hm].height;
                                        }
                                        else if (!Utils::getBit(tile.direction, 1) && cmask.flipY) {
                                            cy = 0;
                                            ch = cmask.collision[hm].height + 1;
                                        }
                                        else if (Utils::getBit(tile.direction, 1) && cmask.flipY) {
                                            cy = 15 - cmask.collision[hm].height;
                                            ch = cmask.collision[hm].height + 1;
                                        }

                                        ypos = yStore + (ch / 2.0);

                                        drawRect((xpos + cx) * zoom, (ypos + cy) * zoom, 15.45,
                                                 1 * zoom, ch * zoom, pixelSolidityClrs[solidity],
                                                 primitiveShader);
                                    }
                                }
                            }
                            spriteShader.use();
                            rectVAO.bind();
                        }
                    }
                }
            }
        }

        // PARALLAX
        if (l == selectedLayer && l > 0) {
            if (background.layers[l - 1].behaviour == 1 || background.layers[l - 1].behaviour == 2) {
                primitiveShader.use();
                primitiveShader.setValue("projection", getProjectionMatrix());
                primitiveShader.setValue("view", QMatrix4x4());
                primitiveShader.setValue("useAlpha", false);
                primitiveShader.setValue("alpha", 1.0f);
                primitiveShader.setValue("projection", matWorld = getProjectionMatrix());
                primitiveShader.setValue("view", matView = QMatrix4x4());
                QMatrix4x4 matModel;
                primitiveShader.setValue("model", matModel);

                QOpenGLVertexArrayObject colVAO;
                colVAO.create();
                colVAO.bind();

                QList<QVector3D> verts;
                if (showParallax) {
                    int id = 0;
                    for (FormatHelpers::Background::ScrollIndexInfo &info :
                         background.layers[l - 1].scrollInfos) {
                        bool isSelected = selectedScrollInfo == id;

                        Vector4<float> clr(1.0f, 1.0f, 0.0f, 1.0f);
                        if (isSelected)
                            clr = Vector4<float>(0.0f, 0.0f, 1.0f, 1.0f);
                        float zpos = (isSelected ? 15.55f : 15.5f);

                        if (background.layers[l - 1].behaviour == 1) {
                            int w = (width * 0x80) * zoom;
                            drawLine(0.0f * zoom, (info.startLine - cam.pos.y) * zoom, zpos,
                                     (w - cam.pos.x) * zoom, (info.startLine - cam.pos.y) * zoom, zpos,
                                     clr, primitiveShader);

                            drawLine(0.0f * zoom, ((info.startLine + info.length) - cam.pos.y) * zoom,
                                     zpos, (w - cam.pos.x) * zoom,
                                     ((info.startLine + info.length) - cam.pos.y) * zoom, zpos, clr,
                                     primitiveShader);
                        }
                        else if (background.layers[l - 1].behaviour == 2) {
                            int h = (height * 0x80) * zoom;
                            drawLine((info.startLine - cam.pos.x) * zoom, 0.0f * zoom, zpos,
                                     (info.startLine - cam.pos.x) * zoom, (h - cam.pos.y) * zoom, zpos,
                                     clr, primitiveShader);

                            drawLine(((info.startLine + info.length) - cam.pos.x) * zoom, 0.0f * zoom,
                                     zpos, ((info.startLine + info.length) - cam.pos.x) * zoom,
                                     (h - cam.pos.y) * zoom, zpos, clr, primitiveShader);
                        }

                        ++id;
                    }
                }
            }
        }
    }

    // ENTITIES
    m_prevSprite = -1;
    spriteShader.use();
    rectVAO.bind();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", false);
    spriteShader.setValue("alpha", 1.0f);
    for (int o = 0; o < scene.objects.count(); ++o) {
        switch (gameType) {
            case ENGINE_v1: break;
            case ENGINE_v2: break;
            case ENGINE_v3: {
                auto &curObj = m_compilerv3.m_objectScriptList[scene.objects[o].type];

                if (curObj.subRSDKDraw.m_scriptCodePtr != SCRIPTDATA_COUNT - 1) {
                    m_compilerv3.m_objectLoop = o;
                    m_compilerv3.processScript(curObj.subRSDKDraw.m_scriptCodePtr,
                                               curObj.subRSDKDraw.m_jumpTablePtr,
                                               Compilerv3::SUB_RSDKDRAW);
                    continue;
                }
                break;
            }
            case ENGINE_v4: {
                auto &curObj = m_compilerv4.m_objectScriptList[scene.objects[o].type];

                if (curObj.eventRSDKDraw.m_scriptCodePtr != SCRIPTDATA_COUNT - 1) {
                    m_compilerv4.m_objectEntityPos = o;
                    m_compilerv4.processScript(curObj.eventRSDKDraw.m_scriptCodePtr,
                                               curObj.eventRSDKDraw.m_jumpTablePtr,
                                               Compilerv4::EVENT_RSDKDRAW);
                    continue;
                }
                break;
            }
        }

        spriteShader.use();
        rectVAO.bind();
        // Draw Object
        float xpos = scene.objects[o].getX() - (cam.pos.x);
        float ypos = scene.objects[o].getY() - (cam.pos.y);
        float zpos = 10.0f;

        int w = objectSprites[0].texturePtr->width(), h = objectSprites[0].texturePtr->height();
        if (m_prevSprite) {
            objectSprites[0].texturePtr->bind();
            m_prevSprite = 0;
        }

        Rect<int> check = Rect<int>();
        check.x         = (int)(xpos + (float)w) * zoom;
        check.y         = (int)(ypos + (float)h) * zoom;
        check.w         = (int)(xpos - (w / 2.0f)) * zoom;
        check.h         = (int)(ypos - (h / 2.0f)) * zoom;
        if (check.x < 0 || check.y < 0 || check.w >= storedW || check.h >= storedH) {
            continue;
        }

        QMatrix4x4 matModel;
        matModel.scale(w * zoom, h * zoom, 1.0f);

        matModel.translate(xpos / (float)w, ypos / (float)h, zpos);
        spriteShader.setValue("model", matModel);

        f->glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // CHUNK PREVIEW
    spriteShader.use();
    rectVAO.bind();
    spriteShader.setValue("useAlpha", true);
    spriteShader.setValue("alpha", 0.75f);
    if (selectedChunk >= 0 && selectedLayer >= 0 && m_selecting && curTool == TOOL_PENCIL) {
        m_tilesetTexture->bind();
        float tx = m_tilePos.x;
        float ty = m_tilePos.y;

        tx *= invZoom();
        ty *= invZoom();

        float tx2 = tx + fmodf(cam.pos.x, 0x80);
        float ty2 = ty + fmodf(cam.pos.y, 0x80);

        // clip to grid
        tx -= fmodf(tx2, 0x80);
        ty -= fmodf(ty2, 0x80);
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                FormatHelpers::Chunks::Tile &tile = chunkset.chunks[selectedChunk].tiles[y][x];

                // Draw Selected Tile Preview
                float xpos = tx + (x * 0x10) + cam.pos.x;
                float ypos = ty + (y * 0x10) + cam.pos.y;
                float zpos = 15.0f;

                xpos -= (cam.pos.x + camOffset.x);
                ypos -= (cam.pos.y + camOffset.y);

                drawTile(xpos, ypos, zpos, 0, tile.tileIndex * 0x10, tile.direction);
            }
        }
    }

    // ENT PREVIEW
    spriteShader.use();
    rectVAO.bind();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", true);
    spriteShader.setValue("alpha", 0.75f);
    if (selectedObject >= 0 && m_selecting && curTool == TOOL_ENTITY) {
        bool flag = false;
        float ex  = m_tilePos.x;
        float ey  = m_tilePos.y;

        ex *= invZoom();
        ey *= invZoom();

        float cx = cam.pos.x;
        float cy = cam.pos.y;
        switch (gameType) {
            case ENGINE_v1: break;
            case ENGINE_v2: break;
            case ENGINE_v3: {
                auto &curObj = m_compilerv3.m_objectScriptList[selectedObject];

                if (curObj.subRSDKDraw.m_scriptCodePtr != SCRIPTDATA_COUNT - 1) {
                    m_compilerv3.m_objectLoop                              = ENTITY_COUNT - 1;
                    m_compilerv3.m_objectEntityList[ENTITY_COUNT - 1].XPos = (ex + cx) * 65536.0f;
                    m_compilerv3.m_objectEntityList[ENTITY_COUNT - 1].YPos = (ey + cy) * 65536.0f;
                    m_compilerv3.processScript(curObj.subRSDKDraw.m_scriptCodePtr,
                                               curObj.subRSDKDraw.m_jumpTablePtr,
                                               Compilerv3::SUB_RSDKDRAW);
                    flag = true;
                }
                break;
            }
            case ENGINE_v4: {
                auto &curObj = m_compilerv4.m_objectScriptList[selectedObject];

                if (curObj.eventRSDKDraw.m_scriptCodePtr != ENTITY_COUNT - 1) {
                    m_compilerv4.m_objectEntityList[ENTITY_COUNT - 1].type = selectedObject;
                    m_compilerv4.m_objectEntityList[ENTITY_COUNT - 1].XPos = (ex + cx) * 65536.0f;
                    m_compilerv4.m_objectEntityList[ENTITY_COUNT - 1].YPos = (ey + cy) * 65536.0f;
                    m_compilerv4.m_objectEntityPos                         = ENTITY_COUNT - 1;
                    m_compilerv4.processScript(curObj.eventRSDKDraw.m_scriptCodePtr,
                                               curObj.eventRSDKDraw.m_jumpTablePtr,
                                               Compilerv4::EVENT_RSDKDRAW);
                    flag = true;
                }
                break;
            }
        }

        if (!flag) {
            // Draw Selected Object Preview
            float xpos = ex;
            float ypos = ey;
            float zpos = 15.0f;

            int w = objectSprites[0].texturePtr->width(), h = objectSprites[0].texturePtr->height();
            objectSprites[0].texturePtr->bind();

            QMatrix4x4 matModel;
            matModel.scale(w * zoom, h * zoom, 1.0f);

            matModel.translate(xpos / (float)w, ypos / (float)h, zpos);
            spriteShader.setValue("model", matModel);

            f->glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    spriteShader.setValue("useAlpha", false);
    spriteShader.setValue("alpha", 1.0f);

    // Draw Retro-Sonic Player spawn point
    if (gameType == ENGINE_v1) {
        float px = scene.m_playerXPos;
        float py = scene.m_playerYPos;

        px *= invZoom();
        py *= invZoom();

        // Draw Player Spawn Preview
        float xpos = px - cam.pos.x;
        float ypos = py - cam.pos.y;
        float zpos = 15.0f;

        int w = m_rsPlayerSprite->width(), h = m_rsPlayerSprite->height();
        m_rsPlayerSprite->bind();

        xpos += w / 2;
        ypos += h / 2;

        QMatrix4x4 matModel;
        matModel.scale(w * zoom, h * zoom, 1.0f);

        matModel.translate(xpos / (float)w, ypos / (float)h, zpos);
        spriteShader.setValue("model", matModel);

        f->glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Selected Ent Box
    rectVAO.bind();
    primitiveShader.use();
    primitiveShader.setValue("projection", getProjectionMatrix());
    primitiveShader.setValue("view", QMatrix4x4());
    primitiveShader.setValue("useAlpha", false);
    primitiveShader.setValue("alpha", 1.0f);
    QMatrix4x4 matModel;
    primitiveShader.setValue("model", matModel);
    if (m_selectedEntity >= 0) {
        FormatHelpers::Scene::Object &object = scene.objects[m_selectedEntity];
        int w = objectSprites[0].texturePtr->width(), h = objectSprites[0].texturePtr->height();
        objectSprites[0].texturePtr->bind();

        drawRect(((object.getX() - cam.pos.x) - (w / 2)) * zoom,
                 ((object.getY() - cam.pos.y) - (h / 2)) * zoom, 15.7f, w * zoom, h * zoom,
                 Vector4<float>(1.0f, 1.0f, 1.0f, 1.0f), primitiveShader, true);
    }

    if (showChunkGrid) {
        rectVAO.bind();

        float camX = cam.pos.x;
        float camY = cam.pos.y;

        for (int y = camY - ((int)camY % 0x80); y < (camY + storedH) * (zoom < 1.0f ? invZoom() : 1.0f);
             y += 0x80) {
            drawLine((camX - camX) * zoom, (y - camY) * zoom, 15.6f,
                     (((camX + storedW * invZoom())) - camX) * zoom, (y - camY) * zoom, 15.6f,
                     Vector4<float>(1.0f, 1.0f, 1.0f, 1.0f), primitiveShader);
        }

        for (int x = camX - ((int)camX % 0x80); x < (camX + storedW) * (zoom < 1.0f ? invZoom() : 1.0f);
             x += 0x80) {
            drawLine((x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom, (camY - camY) * zoom, 15.6f,
                     (x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom,
                     (((camY + storedH * invZoom())) - camY) * zoom, 15.6f,
                     Vector4<float>(1.0f, 1.0f, 1.0f, 1.0f), primitiveShader);
        }
    }

    if (showTileGrid) {
        rectVAO.bind();

        float camX = cam.pos.x;
        float camY = cam.pos.y;

        for (int y = camY - ((int)camY % 0x10); y < (camY + storedH) * (zoom < 1.0f ? invZoom() : 1.0f);
             y += 0x10) {
            drawLine((camX - camX) * zoom, (y - camY) * zoom, 15.6f,
                     (((camX + storedW * invZoom())) - camX) * zoom, (y - camY) * zoom, 15.6f,
                     Vector4<float>(1.0f, 1.0f, 1.0f, 1.0f), primitiveShader);
        }

        for (int x = camX - ((int)camX % 0x10); x < (camX + storedW) * (zoom < 1.0f ? invZoom() : 1.0f);
             x += 0x10) {
            drawLine((x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom, (camY - camY) * zoom, 15.6f,
                     (x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom,
                     (((camY + storedH * invZoom())) - camY) * zoom, 15.6f,
                     Vector4<float>(1.0f, 1.0f, 1.0f, 1.0f), primitiveShader);
        }
    }

    if (showPixelGrid && zoom >= 4.0f) {
        // f->glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
        rectVAO.bind();
        QList<QVector3D> verts;
        primitiveShader.use();
        primitiveShader.setValue("colour", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

        float camX = cam.pos.x;
        float camY = cam.pos.y;

        for (int y = camY; y < (camY + storedH) * (zoom < 1.0f ? invZoom() : 1.0f); ++y) {
            verts.append(QVector3D((camX - camX) * zoom, (y - camY) * zoom, 15.6f));
            verts.append(
                QVector3D((((camX + storedW * invZoom())) - camX) * zoom, (y - camY) * zoom, 15.6f));
        }

        for (int x = camX; x < (camX + storedW) * (zoom < 1.0f ? invZoom() : 1.0f); ++x) {
            verts.append(QVector3D((x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom,
                                   (camY - camY) * zoom, 15.6f));
            verts.append(QVector3D((x + (zoom <= 1.0f ? 1.0f : 0.0f) - camX) * zoom,
                                   (((camY + storedH * invZoom())) - camY) * zoom, 15.6f));
        }

        QVector3D *vertsPtr = new QVector3D[(uint)verts.count()];
        for (int v = 0; v < verts.count(); ++v) vertsPtr[v] = verts[v];

        QOpenGLVertexArrayObject gridVAO;
        gridVAO.create();
        gridVAO.bind();

        QOpenGLBuffer gridVBO;
        gridVBO.create();
        gridVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
        gridVBO.bind();
        gridVBO.allocate(vertsPtr, verts.count() * sizeof(QVector3D));
        primitiveShader.enableAttributeArray(0);
        primitiveShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

        f->glDrawArrays(GL_LINES, 0, verts.count());

        delete[] vertsPtr;

        gridVAO.release();
        gridVBO.release();
    }
}

void SceneViewer::unloadScene()
{
    // QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    if (m_tilesetTexture) {
        m_tilesetTexture->destroy();
        delete m_tilesetTexture;
    }
    m_tilesetTexture = nullptr;
    tiles.clear();

    chunks.clear();

    for (int o = 0; o < objectSprites.count(); ++o) {
        objectSprites[o].texturePtr->destroy();
        delete objectSprites[o].texturePtr;
        objectSprites[o].name = "";
    }
    objectSprites.clear();

    if (m_rsPlayerSprite) {
        m_rsPlayerSprite->destroy();
        delete m_rsPlayerSprite;
    }

    m_compilerv3.clearScriptData();
    m_compilerv4.clearScriptData();

    cam                = SceneCamera();
    selectedChunk      = -1;
    m_selectedEntity   = -1;
    selectedLayer      = -1;
    selectedScrollInfo = -1;
    selectedObject     = -1;
    m_selecting        = false;
}

void SceneViewer::initializeGL()
{
    // QOpenGLFunctions::initializeOpenGLFunctions();

    // Set up the rendering context, load shaders and other resources, etc.
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    QSurfaceFormat fmt        = glContext->format();
    qDebug() << "Widget Using OpenGL " + QString::number(fmt.majorVersion()) + "."
                    + QString::number(fmt.minorVersion());

    const unsigned char *vendor     = f->glGetString(GL_VENDOR);
    const unsigned char *renderer   = f->glGetString(GL_RENDERER);
    const unsigned char *version    = f->glGetString(GL_VERSION);
    const unsigned char *sdrVersion = f->glGetString(GL_SHADING_LANGUAGE_VERSION);
    const unsigned char *extensions = f->glGetString(GL_EXTENSIONS);

    QString vendorStr     = reinterpret_cast<const char *>(vendor);
    QString rendererStr   = reinterpret_cast<const char *>(renderer);
    QString versionStr    = reinterpret_cast<const char *>(version);
    QString sdrVersionStr = reinterpret_cast<const char *>(sdrVersion);
    QString extensionsStr = reinterpret_cast<const char *>(extensions);

    qDebug() << "OpenGL Details";
    qDebug() << "Vendor:       " + vendorStr;
    qDebug() << "Renderer:     " + rendererStr;
    qDebug() << "Version:      " + versionStr;
    qDebug() << "GLSL version: " + sdrVersionStr;
    qDebug() << "Extensions:   " + extensionsStr;
    qDebug() << (QOpenGLContext::currentContext()->isOpenGLES() ? "Using OpenGLES" : "Using OpenGL");
    qDebug() << (QOpenGLContext::currentContext()->isValid() ? "Is valid" : "Not valid");

    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    primitiveShader.loadShader(":/shaders/primitive.glv", QOpenGLShader::Vertex);
    primitiveShader.loadShader(":/shaders/primitive.glf", QOpenGLShader::Fragment);
    primitiveShader.link();
    primitiveShader.use();

    spriteShader.loadShader(":/shaders/sprite.glv", QOpenGLShader::Vertex);
    spriteShader.loadShader(":/shaders/sprite.glf", QOpenGLShader::Fragment);
    spriteShader.link();
    spriteShader.use();

    rectVAO.create();
    rectVAO.bind();

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::StaticDraw);
    tVBO2D.bind();
    tVBO2D.allocate(rectTexCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    // Release (unbind) all
    rectVAO.release();
    vVBO2D.release();
}

void SceneViewer::resizeGL(int w, int h)
{
    storedW             = w;
    storedH             = h;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, w, h);

    // m_sbHorizontal->setMaximum((m_scene.m_sceneConfig.m_camBounds.w * 0x80) - m_storedW);
    // m_sbVertical->setMaximum((m_scene.m_sceneConfig.m_camBounds.h * 0x80) - m_storedH);
}

void SceneViewer::paintGL()
{
    // Draw the scene:
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    f->glClearColor(m_bgColour.r / 255.0f, m_bgColour.g / 255.0f, m_bgColour.b / 255.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene();
}

int SceneViewer::addGraphicsFile(char *sheetPath)
{
    QString path = m_dataPath + "/Sprites/" + sheetPath;
    if (!QFile::exists(path))
        return 0;

    for (int i = 1; i < objectSprites.count(); ++i) {
        if (QString(sheetPath) == objectSprites[i].name)
            return i;
    }

    int sheetID = -1;
    for (int i = 1; i < objectSprites.count(); ++i) {
        if (objectSprites[i].name == "")
            sheetID = i;
    }

    if (sheetID >= 0) {
        QImage sheet(path);
        TextureInfo tex;
        tex.name               = QString(sheetPath);
        tex.texturePtr         = createTexture(sheet);
        objectSprites[sheetID] = tex;
        return sheetID;
    }
    else {
        QImage sheet(path);
        int cnt = objectSprites.count();
        TextureInfo tex;
        tex.name       = QString(sheetPath);
        tex.texturePtr = createTexture(sheet);
        objectSprites.append(tex);
        return cnt;
    }
}

void SceneViewer::removeGraphicsFile(char *sheetPath, int slot)
{
    if (slot >= 0) {
        objectSprites[slot].texturePtr->destroy();
        delete objectSprites[slot].texturePtr;
        objectSprites[slot].name = "";
    }
    else {
        for (int i = 1; i < objectSprites.count(); ++i) {
            if (QString(sheetPath) == objectSprites[i].name) {
                objectSprites[slot].texturePtr->destroy();
                delete objectSprites[slot].texturePtr;
                objectSprites[slot].name = "";
            }
        }
    }
}

void SceneViewer::drawTile(float XPos, float YPos, float ZPos, int tileX, int tileY, byte direction)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    // Draw Sprite
    float w = m_tilesetTexture->width(), h = m_tilesetTexture->height();

    spriteShader.use();
    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    float tx = tileX / w;
    float ty = tileY / h;
    float tw = 0x10 / w;
    float th = 0x10 / h;

    QVector2D *texCoords = nullptr;
    switch (direction) {
        case 0:
        default: {
            QVector2D tc[] = {
                QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
                QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
            };
            texCoords = tc;
            break;
        }
        case 1: {
            QVector2D tc[] = {
                QVector2D(tx + tw, ty), QVector2D(tx, ty),           QVector2D(tx, ty + th),
                QVector2D(tx, ty + th), QVector2D(tx + tw, ty + th), QVector2D(tx + tw, ty),
            };
            texCoords = tc;
            break;
        }
        case 2: {
            QVector2D tc[] = {
                QVector2D(tx, ty + th), QVector2D(tx + tw, ty + th), QVector2D(tx + tw, ty),
                QVector2D(tx + tw, ty), QVector2D(tx, ty),           QVector2D(tx, ty + th),
            };
            texCoords = tc;
            break;
        }
        case 3: {
            QVector2D tc[] = {
                QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
                QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
            };
            texCoords = tc;
            break;
        }
    }

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    tVBO2D.bind();
    tVBO2D.allocate(texCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    QMatrix4x4 matModel;
    matModel.scale(0x10 * zoom, 0x10 * zoom, 1.0f);

    matModel.translate((XPos + (0x10 / 2)) / (float)0x10, (YPos + (0x10 / 2)) / (float)0x10, ZPos);
    spriteShader.setValue("model", matModel);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);

    vao.release();
    tVBO2D.destroy();
    vVBO2D.destroy();
    vao.destroy();
}

int sprDraws = 0;
void SceneViewer::drawSprite(int XPos, int YPos, int width, int height, int sprX, int sprY, int sheetID)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (sheetID != m_prevSprite)
        sprDraws = 0;

    // Draw Sprite
    float xpos = XPos - cam.pos.x;
    float ypos = YPos - cam.pos.y;
    float zpos = 10.0f + (sprDraws * 0.001f);

    if (sheetID != m_prevSprite) {
        objectSprites[sheetID].texturePtr->bind();
        m_prevSprite = sheetID;
        sprDraws     = 0;
    }
    float w = objectSprites[sheetID].texturePtr->width(),
          h = objectSprites[sheetID].texturePtr->height();

    Rect<int> check = Rect<int>();
    check.x         = (int)(xpos + (float)w) * zoom;
    check.y         = (int)(ypos + (float)h) * zoom;
    check.w         = (int)(xpos - (w / 2.0f)) * zoom;
    check.h         = (int)(ypos - (h / 2.0f)) * zoom;
    if (check.x < 0 || check.y < 0 || check.w >= storedW || check.h >= storedH || !sheetID) {
        return;
    }

    spriteShader.use();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", false);
    spriteShader.setValue("alpha", 1.0f);
    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    double tx = sprX / w;
    double ty = sprY / h;
    double tw = (sprX + width) / w;
    double th = (sprY + height) / h;

    const QVector2D texCoords[] = {
        QVector2D(tx, ty), QVector2D(tw, ty), QVector2D(tw, th),
        QVector2D(tw, th), QVector2D(tx, th), QVector2D(tx, ty),
    };

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    tVBO2D.bind();
    tVBO2D.allocate(texCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    QMatrix4x4 matModel;
    matModel.scale(width * zoom, height * zoom, 1.0f);

    matModel.translate((xpos + (width / 2)) / (float)width, (ypos + (height / 2)) / (float)height,
                       zpos);
    spriteShader.setValue("model", matModel);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);
    sprDraws++;
}

void SceneViewer::drawSpriteFlipped(int XPos, int YPos, int width, int height, int sprX, int sprY,
                                    int direction, int sheetID)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (sheetID != m_prevSprite)
        sprDraws = 0;

    // Draw Sprite
    float xpos = XPos - cam.pos.x;
    float ypos = YPos - cam.pos.y;
    float zpos = 10.0f + (sprDraws * 0.001f);

    if (sheetID != m_prevSprite) {
        objectSprites[sheetID].texturePtr->bind();
        m_prevSprite = sheetID;
    }
    float w = objectSprites[sheetID].texturePtr->width(),
          h = objectSprites[sheetID].texturePtr->height();

    Rect<int> check = Rect<int>();
    check.x         = (int)(xpos + (float)w) * zoom;
    check.y         = (int)(ypos + (float)h) * zoom;
    check.w         = (int)(xpos - (w / 2.0f)) * zoom;
    check.h         = (int)(ypos - (h / 2.0f)) * zoom;
    if (check.x < 0 || check.y < 0 || check.w >= storedW || check.h >= storedH || !sheetID) {
        return;
    }

    spriteShader.use();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", false);
    spriteShader.setValue("alpha", 1.0f);
    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    float tx = sprX / w;
    float ty = sprY / h;
    float tw = width / w;
    float th = height / h;

    QVector2D *texCoords = nullptr;
    switch (direction) {
        case 0:
        default: {
            QVector2D tc[] = {
                QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
                QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
            };
            texCoords = tc;
            break;
        }
        case 1: {
            QVector2D tc[] = {
                QVector2D(tx + tw, ty), QVector2D(tx, ty),           QVector2D(tx, ty + th),
                QVector2D(tx, ty + th), QVector2D(tx + tw, ty + th), QVector2D(tx + tw, ty),
            };
            texCoords = tc;
            break;
        }
        case 2: {
            QVector2D tc[] = {
                QVector2D(tx, ty + th), QVector2D(tx + tw, ty + th), QVector2D(tx + tw, ty),
                QVector2D(tx + tw, ty), QVector2D(tx, ty),           QVector2D(tx, ty + th),
            };
            texCoords = tc;
            break;
        }
        case 3: {
            QVector2D tc[] = {
                QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
                QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
            };
            texCoords = tc;
            break;
        }
    }

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    tVBO2D.bind();
    tVBO2D.allocate(texCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    QMatrix4x4 matModel;
    matModel.scale(width * zoom, height * zoom, 1.0f);

    matModel.translate((xpos + (width / 2)) / (float)width, (ypos + (height / 2)) / (float)height,
                       zpos);
    spriteShader.setValue("model", matModel);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);
    sprDraws++;
}

void SceneViewer::drawBlendedSprite(int XPos, int YPos, int width, int height, int sprX, int sprY,
                                    int sheetID)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (sheetID != m_prevSprite)
        sprDraws = 0;

    // Draw Sprite
    float xpos = XPos - cam.pos.x;
    float ypos = YPos - cam.pos.y;
    float zpos = 10.0f + (sprDraws * 0.001f);

    if (sheetID != m_prevSprite) {
        objectSprites[sheetID].texturePtr->bind();
        m_prevSprite = sheetID;
    }
    float w = objectSprites[sheetID].texturePtr->width(),
          h = objectSprites[sheetID].texturePtr->height();

    Rect<int> check = Rect<int>();
    check.x         = (int)(xpos + (float)w) * zoom;
    check.y         = (int)(ypos + (float)h) * zoom;
    check.w         = (int)(xpos - (w / 2.0f)) * zoom;
    check.h         = (int)(ypos - (h / 2.0f)) * zoom;
    if (check.x < 0 || check.y < 0 || check.w >= storedW || check.h >= storedH || !sheetID) {
        return;
    }

    spriteShader.use();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", true);
    spriteShader.setValue("alpha", 0.5f);
    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    float tx = sprX / w;
    float ty = sprY / h;
    float tw = width / w;
    float th = height / h;

    const QVector2D texCoords[] = {
        QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
        QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
    };

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    tVBO2D.bind();
    tVBO2D.allocate(texCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    QMatrix4x4 matModel;
    matModel.scale(width * zoom, height * zoom, 1.0f);

    matModel.translate((xpos + (width / 2)) / (float)width, (ypos + (height / 2)) / (float)height,
                       zpos);
    spriteShader.setValue("model", matModel);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);
    sprDraws++;
}

void SceneViewer::drawAlphaBlendedSprite(int XPos, int YPos, int width, int height, int sprX, int sprY,
                                         int alpha, int sheetID)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (sheetID != m_prevSprite)
        sprDraws = 0;

    // Draw Sprite
    float xpos = XPos - cam.pos.x;
    float ypos = YPos - cam.pos.y;
    float zpos = 10.0f + (sprDraws * 0.001f);

    if (sheetID != m_prevSprite) {
        objectSprites[sheetID].texturePtr->bind();
        m_prevSprite = sheetID;
    }
    float w = objectSprites[sheetID].texturePtr->width(),
          h = objectSprites[sheetID].texturePtr->height();

    Rect<int> check = Rect<int>();
    check.x         = (int)(xpos + (float)w) * zoom;
    check.y         = (int)(ypos + (float)h) * zoom;
    check.w         = (int)(xpos - (w / 2.0f)) * zoom;
    check.h         = (int)(ypos - (h / 2.0f)) * zoom;
    if (check.x < 0 || check.y < 0 || check.w >= storedW || check.h >= storedH || !sheetID) {
        return;
    }

    spriteShader.use();
    spriteShader.setValue("flipX", false);
    spriteShader.setValue("flipY", false);
    spriteShader.setValue("useAlpha", true);
    spriteShader.setValue("alpha", (alpha > 0xFF ? 0xFF : alpha) / 255.0f);
    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    float tx = sprX / w;
    float ty = sprY / h;
    float tw = width / w;
    float th = height / h;

    const QVector2D texCoords[] = {
        QVector2D(tx, ty),           QVector2D(tx + tw, ty), QVector2D(tx + tw, ty + th),
        QVector2D(tx + tw, ty + th), QVector2D(tx, ty + th), QVector2D(tx, ty),
    };

    QOpenGLBuffer vVBO2D;
    vVBO2D.create();
    vVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vVBO2D.bind();
    vVBO2D.allocate(rectVertices, 6 * sizeof(QVector3D));
    spriteShader.enableAttributeArray(0);
    spriteShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

    QOpenGLBuffer tVBO2D;
    tVBO2D.create();
    tVBO2D.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    tVBO2D.bind();
    tVBO2D.allocate(texCoords, 6 * sizeof(QVector2D));
    spriteShader.enableAttributeArray(1);
    spriteShader.setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);

    QMatrix4x4 matModel;
    matModel.scale(width * zoom, height * zoom, 1.0f);

    matModel.translate((xpos + (width / 2)) / (float)width, (ypos + (height / 2)) / (float)height,
                       zpos);
    spriteShader.setValue("model", matModel);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);
    sprDraws++;
}
