#include "components/CameraComponent.hpp"

#include <gtest/gtest.h>

TEST(CameraComponent, DefaultValues)
{
    CameraComponent camera;

    EXPECT_FLOAT_EQ(camera.x, 0.0F);
    EXPECT_FLOAT_EQ(camera.y, 0.0F);
    EXPECT_FLOAT_EQ(camera.zoom, 1.0F);
    EXPECT_FLOAT_EQ(camera.offsetX, 0.0F);
    EXPECT_FLOAT_EQ(camera.offsetY, 0.0F);
    EXPECT_FLOAT_EQ(camera.rotation, 0.0F);
    EXPECT_TRUE(camera.active);
}

TEST(CameraComponent, CreateWithPosition)
{
    CameraComponent camera = CameraComponent::create(100.0F, 200.0F);

    EXPECT_FLOAT_EQ(camera.x, 100.0F);
    EXPECT_FLOAT_EQ(camera.y, 200.0F);
    EXPECT_FLOAT_EQ(camera.zoom, 1.0F);
}

TEST(CameraComponent, CreateWithZoom)
{
    CameraComponent camera = CameraComponent::create(100.0F, 200.0F, 2.0F);

    EXPECT_FLOAT_EQ(camera.x, 100.0F);
    EXPECT_FLOAT_EQ(camera.y, 200.0F);
    EXPECT_FLOAT_EQ(camera.zoom, 2.0F);
}

TEST(CameraComponent, SetPosition)
{
    CameraComponent camera;
    camera.setPosition(150.0F, 250.0F);

    EXPECT_FLOAT_EQ(camera.x, 150.0F);
    EXPECT_FLOAT_EQ(camera.y, 250.0F);
}

TEST(CameraComponent, Move)
{
    CameraComponent camera = CameraComponent::create(100.0F, 100.0F);
    camera.move(50.0F, -25.0F);

    EXPECT_FLOAT_EQ(camera.x, 150.0F);
    EXPECT_FLOAT_EQ(camera.y, 75.0F);
}

TEST(CameraComponent, SetZoom)
{
    CameraComponent camera;
    camera.setZoom(3.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 3.0F);
}

TEST(CameraComponent, SetZoomRejectsNegative)
{
    CameraComponent camera;
    camera.setZoom(-1.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 1.0F);
}

TEST(CameraComponent, SetZoomRejectsZero)
{
    CameraComponent camera;
    camera.setZoom(0.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 1.0F);
}

TEST(CameraComponent, SetOffset)
{
    CameraComponent camera;
    camera.setOffset(10.0F, -5.0F);

    EXPECT_FLOAT_EQ(camera.offsetX, 10.0F);
    EXPECT_FLOAT_EQ(camera.offsetY, -5.0F);
}

TEST(CameraComponent, SetRotation)
{
    CameraComponent camera;
    camera.setRotation(45.0F);

    EXPECT_FLOAT_EQ(camera.rotation, 45.0F);
}

TEST(CameraComponent, Rotate)
{
    CameraComponent camera;
    camera.setRotation(30.0F);
    camera.rotate(15.0F);

    EXPECT_FLOAT_EQ(camera.rotation, 45.0F);
}

TEST(CameraComponent, Reset)
{
    CameraComponent camera = CameraComponent::create(100.0F, 200.0F, 2.0F);
    camera.setOffset(10.0F, 20.0F);
    camera.setRotation(45.0F);

    camera.reset();

    EXPECT_FLOAT_EQ(camera.x, 0.0F);
    EXPECT_FLOAT_EQ(camera.y, 0.0F);
    EXPECT_FLOAT_EQ(camera.zoom, 1.0F);
    EXPECT_FLOAT_EQ(camera.offsetX, 0.0F);
    EXPECT_FLOAT_EQ(camera.offsetY, 0.0F);
    EXPECT_FLOAT_EQ(camera.rotation, 0.0F);
}

TEST(CameraComponent, ClampZoomMin)
{
    CameraComponent camera;
    camera.setZoom(0.3F);
    camera.clampZoom(0.5F, 3.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 0.5F);
}

TEST(CameraComponent, ClampZoomMax)
{
    CameraComponent camera;
    camera.setZoom(5.0F);
    camera.clampZoom(0.5F, 3.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 3.0F);
}

TEST(CameraComponent, ClampZoomWithinRange)
{
    CameraComponent camera;
    camera.setZoom(2.0F);
    camera.clampZoom(0.5F, 3.0F);

    EXPECT_FLOAT_EQ(camera.zoom, 2.0F);
}

TEST(CameraComponent, ActiveFlag)
{
    CameraComponent camera;
    EXPECT_TRUE(camera.active);

    camera.active = false;
    EXPECT_FALSE(camera.active);
}
