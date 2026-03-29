#include "ui/video_opengl_widget.h"

#include <array>
#include <cmath>

#include <QThread>

namespace VisionCursor
{
namespace
{

struct VertexData
{
    float x;
    float y;
    float u;
    float v;
};

}      // namespace

VideoOpenGLWidget::VideoOpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setMinimumSize(320, 240);
    setAutoFillBackground(false);
}

VideoOpenGLWidget::~VideoOpenGLWidget()
{
    if (context() != nullptr)
    {
        makeCurrent();
        releaseGlResources();
        doneCurrent();
    }
}

void VideoOpenGLWidget::setFrame(const FramePtr& frame)
{
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        current_frame_ = frame;
    }

    if (QThread::currentThread() == thread())
    {
        update();
    }
    else
    {
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}

void VideoOpenGLWidget::clearFrame()
{
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        current_frame_.reset();
    }

    if (QThread::currentThread() == thread())
    {
        update();
    }
    else
    {
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
}

void VideoOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    initShaders();
    initGeometry();

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);

    gl_inited_ = true;
}

void VideoOpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void VideoOpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    FramePtr frame;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        frame = current_frame_;
    }

    if (!frame || frame->getData() == nullptr || frame->getWidth() <= 0 || frame->getHeight() <= 0)
    {
        return;
    }

    updateTextureIfNeeded(frame);

    const float widget_w = static_cast<float>(width());
    const float widget_h = static_cast<float>(height());
    const float frame_w  = static_cast<float>(frame->getWidth());
    const float frame_h  = static_cast<float>(frame->getHeight());

    if (widget_w <= 0.0f || widget_h <= 0.0f || frame_w <= 0.0f || frame_h <= 0.0f)
    {
        return;
    }

    const float widget_aspect = widget_w / widget_h;
    const float frame_aspect  = frame_w / frame_h;

    float draw_w = 1.0f;
    float draw_h = 1.0f;

    if (frame_aspect > widget_aspect)
    {
        draw_h = widget_aspect / frame_aspect;
    }
    else
    {
        draw_w = frame_aspect / widget_aspect;
    }

    const std::array<VertexData, 4> vertices = {VertexData{-draw_w, -draw_h, 0.0f, 1.0f},
                                                VertexData{draw_w, -draw_h, 1.0f, 1.0f},
                                                VertexData{-draw_w, draw_h, 0.0f, 0.0f},
                                                VertexData{draw_w, draw_h, 1.0f, 0.0f}};

    vao_.bind();
    vbo_.bind();
    vbo_.write(0, vertices.data(), static_cast<int>(sizeof(VertexData) * vertices.size()));

    program_.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    program_.setUniformValue("uTexture", 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    program_.release();
    vbo_.release();
    vao_.release();
}

void VideoOpenGLWidget::initShaders()
{
    static const char* vertex_shader_source = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

    static const char* fragment_shader_source = R"(#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = texture(uTexture, vTexCoord);
}
)";

    program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    program_.link();
}

void VideoOpenGLWidget::initGeometry()
{
    vao_.create();
    vao_.bind();

    vbo_.create();
    vbo_.bind();
    vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    const std::array<VertexData, 4> initial_vertices = {VertexData{-1.0f, -1.0f, 0.0f, 1.0f},
                                                        VertexData{1.0f, -1.0f, 1.0f, 1.0f},
                                                        VertexData{-1.0f, 1.0f, 0.0f, 0.0f},
                                                        VertexData{1.0f, 1.0f, 1.0f, 0.0f}};

    vbo_.allocate(initial_vertices.data(), static_cast<int>(sizeof(VertexData) * initial_vertices.size()));

    program_.bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(offsetof(VertexData, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), reinterpret_cast<void*>(offsetof(VertexData, u)));

    program_.release();
    vbo_.release();
    vao_.release();
}

void VideoOpenGLWidget::updateTextureIfNeeded(const FramePtr& frame)
{
    if (!gl_inited_ || texture_id_ == 0 || !frame || frame->getData() == nullptr)
    {
        return;
    }

    const int frame_w = frame->getWidth();
    const int frame_h = frame->getHeight();

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (texture_width_ != frame_w || texture_height_ != frame_h)
    {
        texture_width_  = frame_w;
        texture_height_ = frame_h;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_w, frame_h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->getData());
    }
    else
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_w, frame_h, GL_RGB, GL_UNSIGNED_BYTE, frame->getData());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoOpenGLWidget::releaseGlResources()
{
    if (texture_id_ != 0)
    {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }

    if (vbo_.isCreated())
    {
        vbo_.destroy();
    }

    if (vao_.isCreated())
    {
        vao_.destroy();
    }

    if (program_.isLinked())
    {
        program_.removeAllShaders();
    }

    texture_width_  = 0;
    texture_height_ = 0;
    gl_inited_      = false;
}

}      // namespace VisionCursor