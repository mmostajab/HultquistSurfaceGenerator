/*
 camera.h
 OpenGL Camera Code
 Capable of 2 modes, orthogonal, and free
 Quaternion camera code adapted from: http://hamelot.co.uk/visualization/opengl-camera/
 Written by Hammad Mazhar
 *** Expanded by Morteza Mostajab
 */
#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <GL/glew.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();

    void init(const glm::vec3& camera_pos, const glm::vec3& camera_look_at, const glm::vec3& camera_up, glm::ivec2& vp_size, const double& fov, const double& near_clip, const double& far_clip);
    
    void moveCamera(const double& speed, const bool& forward, const bool& backward);
    void Camera::rotateCamera(const double& speed, const double& x, const double& y);

    glm::mat4 getViewMat();
    glm::mat4 getProjMat();

    bool isMoved();

    ~Camera();

private:

    glm::ivec2 m_viewportsize;
   
    double m_aspect_ratio;
    double m_fov;
    double m_near_clip;
    double m_far_clip;

    glm::vec3 m_camera_pos;
    glm::vec3 m_camera_lookat;
    glm::vec3 m_camera_up;

    glm::vec3 m_camera_lookat_vec;
    glm::vec3 m_camera_right_vec;
    glm::vec3 m_camera_up_vec;

    glm::mat4 m_projmat;
    glm::mat4 m_viewmat;

    bool moved;
};

#endif