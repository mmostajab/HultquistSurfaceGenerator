#include "Camera.h"

Camera::Camera() {

}

void Camera::init(const glm::vec3& camera_pos, const glm::vec3& camera_lookat, const glm::vec3& camera_up, glm::ivec2& vp_size, const double& fov, const double& near_clip, const double& far_clip) {
    m_camera_pos    = camera_pos;
    m_camera_lookat = camera_lookat;
    m_camera_up     = camera_up;
    m_viewportsize  = vp_size;
    m_fov           = fov;
    m_far_clip      = far_clip;
    m_near_clip     = near_clip;

    m_camera_lookat_vec = (m_camera_lookat - m_camera_pos);
    m_camera_lookat_vec = glm::normalize(m_camera_lookat_vec);
    m_camera_right_vec = glm::cross(m_camera_lookat_vec, camera_up);
    m_camera_up = glm::normalize(m_camera_up);
    m_camera_up_vec = glm::cross(m_camera_right_vec, m_camera_lookat_vec);
    m_camera_up_vec = glm::normalize(m_camera_up_vec);

    moved = true;
}

void Camera::moveCamera(const double& speed, const bool& forward, const bool& backward){
    glm::vec3 lookat_dir = (m_camera_lookat - m_camera_pos);
    glm::normalize(lookat_dir);
    glm::vec3 displacement = lookat_dir;
    displacement *= speed;
    
    if (forward) {
        m_camera_pos += displacement;
        moved = true;
    }

    if (backward) {
        m_camera_pos -= displacement;
        moved = true;
    }
}

void Camera::rotateCamera(const double& speed, const double& x, const double& y) {
    float r = glm::length(m_camera_lookat - m_camera_pos);
    m_camera_pos += (m_camera_right_vec * (float)x + m_camera_up_vec * (float)y) * (float)speed * r;
    m_camera_lookat_vec = m_camera_lookat - m_camera_pos;
    m_camera_lookat_vec = glm::normalize(m_camera_lookat_vec);
    m_camera_pos = m_camera_lookat - r * m_camera_lookat_vec;

    m_camera_right_vec = cross(m_camera_lookat_vec, m_camera_up_vec);
    glm::normalize(m_camera_right_vec);
    m_camera_up_vec = cross(m_camera_right_vec, m_camera_lookat_vec);
    glm::normalize(m_camera_up_vec);
    moved = true;
}

glm::mat4 Camera::getViewMat() {
    return glm::lookAt(m_camera_pos, m_camera_lookat, m_camera_up_vec);
}

glm::mat4 Camera::getProjMat() {
    return glm::perspective(m_fov, (double)m_viewportsize.y / m_viewportsize.x, m_near_clip, m_far_clip);
}

bool Camera::isMoved() {
    bool retval = moved;
    moved = false;
    return retval;
}

Camera::~Camera() {

}