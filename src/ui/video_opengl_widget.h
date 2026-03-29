#pragma once

#include <mutex>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QMetaObject>

#include "common/common.h"

namespace VisionCursor
{

class VideoOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit VideoOpenGLWidget(QWidget* parent = nullptr);
    ~VideoOpenGLWidget() override;

public:
    void setFrame(const FramePtr& frame);
    void clearFrame();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initShaders();
    void initGeometry();
    void updateTextureIfNeeded(const FramePtr& frame);
    void releaseGlResources();

private:
    std::mutex frame_mutex_;
    FramePtr current_frame_;

    QOpenGLShaderProgram program_;
    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject vao_;

    GLuint texture_id_ = 0;
    int texture_width_ = 0;
    int texture_height_ = 0;

    bool gl_inited_ = false;
};

} // namespace VisionCursor