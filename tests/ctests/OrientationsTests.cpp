#include "gtest/gtest.h"

#include "ale/Orientations.h"

#include <cmath>
#include <exception>

using namespace std;
using namespace ale;

class OrientationTest : public ::testing::Test {
  protected:
    void SetUp() override {
      rotations.push_back(Rotation( 0.5, 0.5, 0.5, 0.5));
      rotations.push_back(Rotation(-0.5, 0.5, 0.5, 0.5));
      rotations.push_back(Rotation( 1.0, 0.0, 0.0, 0.0));
      times.push_back(0);
      times.push_back(2);
      times.push_back(4);
      avs.push_back(Vec3d(2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI));
      avs.push_back(Vec3d(2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI));
      avs.push_back(Vec3d(2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI));
      orientations = Orientations(rotations, times, avs);
    }

    vector<Rotation> rotations;
    vector<double> times;
    vector<Vec3d> avs;
    Orientations orientations;
};

class ConstOrientationTest : public OrientationTest{
  protected:
    void SetUp() override {
      OrientationTest::SetUp();
      constRotation = Rotation(0, 1, 0, 0);
      vector<int> constFrames = {-74021, -74020, -74000};
      vector<int> timeDepFrames = {-74000, -74900, 1};
      constOrientations = Orientations(rotations, times, avs, constRotation, constFrames, timeDepFrames);
    }

    Rotation constRotation;
    Orientations constOrientations;
};

TEST_F(OrientationTest, ConstructorAccessors) {
  vector<Rotation> outputRotations = orientations.getRotations();
  vector<double> outputTimes = orientations.getTimes();
  vector<Vec3d> outputAvs = orientations.getAngularVelocities();
  ASSERT_EQ(outputRotations.size(), rotations.size());
  for (size_t i = 0; i < outputRotations.size(); i++) {
    vector<double> quats = rotations[i].toQuaternion();
    vector<double> outputQuats = outputRotations[i].toQuaternion();
    EXPECT_EQ(outputQuats[0], quats[0]);
    EXPECT_EQ(outputQuats[1], quats[1]);
    EXPECT_EQ(outputQuats[2], quats[2]);
    EXPECT_EQ(outputQuats[3], quats[3]);
  }
  ASSERT_EQ(outputTimes.size(), times.size());
  for (size_t i = 0; i < outputTimes.size(); i++) {
    EXPECT_EQ(outputTimes[0], times[0]);
  }
  for (size_t i = 0; i < outputAvs.size(); i++) {
    EXPECT_EQ(outputAvs[i].x, avs[i].x);
    EXPECT_EQ(outputAvs[i].y, avs[i].y);
    EXPECT_EQ(outputAvs[i].z, avs[i].z);
  }
}

TEST_F(OrientationTest, Interpolate) {
  Rotation interpRotation = orientations.interpolate(0.25);
  vector<double> quat = interpRotation.toQuaternion();
  ASSERT_EQ(quat.size(), 4);
  EXPECT_NEAR(quat[0], cos(M_PI * 3.0/8.0), 1e-10);
  EXPECT_NEAR(quat[1], sin(M_PI * 3.0/8.0) * 1/sqrt(3.0), 1e-10);
  EXPECT_NEAR(quat[2], sin(M_PI * 3.0/8.0) * 1/sqrt(3.0), 1e-10);
  EXPECT_NEAR(quat[3], sin(M_PI * 3.0/8.0) * 1/sqrt(3.0), 1e-10);
}

TEST_F(OrientationTest, InterpolateAtRotation) {
  Rotation interpRotation = orientations.interpolate(0.0);
  vector<double> quat = interpRotation.toQuaternion();
  ASSERT_EQ(quat.size(), 4);
  EXPECT_NEAR(quat[0], 0.5, 1e-10);
  EXPECT_NEAR(quat[1], 0.5, 1e-10);
  EXPECT_NEAR(quat[2], 0.5, 1e-10);
  EXPECT_NEAR(quat[3], 0.5, 1e-10);
}

TEST_F(OrientationTest, InterpolateAv) {
  Vec3d interpAv = orientations.interpolateAV(0.25);
  EXPECT_NEAR(interpAv.x, 2.0 / 3.0 * M_PI, 1e-10);
  EXPECT_NEAR(interpAv.y, 2.0 / 3.0 * M_PI, 1e-10);
  EXPECT_NEAR(interpAv.z, 2.0 / 3.0 * M_PI, 1e-10);
}

TEST_F(OrientationTest, RotateAt) {
  Vec3d rotatedX = orientations.rotateVectorAt(0.0, Vec3d(1.0, 0.0, 0.0));
  EXPECT_NEAR(rotatedX.x, 0.0, 1e-10);
  EXPECT_NEAR(rotatedX.y, 1.0, 1e-10);
  EXPECT_NEAR(rotatedX.z, 0.0, 1e-10);
  Vec3d rotatedY = orientations.rotateVectorAt(0.0, Vec3d(0.0, 1.0, 0.0));
  EXPECT_NEAR(rotatedY.x, 0.0, 1e-10);
  EXPECT_NEAR(rotatedY.y, 0.0, 1e-10);
  EXPECT_NEAR(rotatedY.z, 1.0, 1e-10);
  Vec3d rotatedZ = orientations.rotateVectorAt(0.0, Vec3d(0.0, 0.0, 1.0));
  EXPECT_NEAR(rotatedZ.x, 1.0, 1e-10);
  EXPECT_NEAR(rotatedZ.y, 0.0, 1e-10);
  EXPECT_NEAR(rotatedZ.z, 0.0, 1e-10);
}

TEST_F(OrientationTest, RotationMultiplication) {
  vector<Rotation> originalRotations = orientations.getRotations();
  vector<double> originalConstQuats = orientations.getConstantRotation().toQuaternion();
  Rotation constRot( 0.5, 0.5, 0.5, 0.5);
  vector<vector<double>> expectedQuats = {
    {-0.5, 0.5, 0.5, 0.5},
    {-1.0, 0.0, 0.0, 0.0},
    { 0.5, 0.5, 0.5, 0.5}
  };

  Orientations rightMultiplied = orientations * constRot;
  vector<Rotation> outputRightRotations = rightMultiplied.getRotations();
  ASSERT_EQ(expectedQuats.size(), outputRightRotations.size());
  for (size_t i = 0; i < outputRightRotations.size(); i++) {
    vector<double> quats = outputRightRotations[i].toQuaternion();
    EXPECT_EQ(expectedQuats[i][0], quats[0]);
    EXPECT_EQ(expectedQuats[i][1], quats[1]);
    EXPECT_EQ(expectedQuats[i][2], quats[2]);
    EXPECT_EQ(expectedQuats[i][3], quats[3]);
  }
  vector<double> outputRightConstQuats = rightMultiplied.getConstantRotation().toQuaternion();
  EXPECT_EQ(originalConstQuats[0], outputRightConstQuats[0]);
  EXPECT_EQ(originalConstQuats[1], outputRightConstQuats[1]);
  EXPECT_EQ(originalConstQuats[2], outputRightConstQuats[2]);
  EXPECT_EQ(originalConstQuats[3], outputRightConstQuats[3]);

  Orientations leftMultiplied = constRot * orientations;
  vector<Rotation> outputLeftRotations = leftMultiplied.getRotations();
  ASSERT_EQ(originalRotations.size(), outputLeftRotations.size());
  for (size_t i = 0; i < outputLeftRotations.size(); i++) {
    vector<double> originalQuats = originalRotations[i].toQuaternion();
    vector<double> quats = outputLeftRotations[i].toQuaternion();
    EXPECT_EQ(originalQuats[0], quats[0]);
    EXPECT_EQ(originalQuats[1], quats[1]);
    EXPECT_EQ(originalQuats[2], quats[2]);
    EXPECT_EQ(originalQuats[3], quats[3]);
  }
  vector<double> outputLeftConstQuats = leftMultiplied.getConstantRotation().toQuaternion();
  EXPECT_EQ(0.5, outputLeftConstQuats[0]);
  EXPECT_EQ(0.5, outputLeftConstQuats[1]);
  EXPECT_EQ(0.5, outputLeftConstQuats[2]);
  EXPECT_EQ(0.5, outputLeftConstQuats[3]);
}

TEST_F(OrientationTest, OrientationMultiplication) {
  Orientations duplicateOrientations(orientations);
  orientations *= duplicateOrientations;
  vector<Rotation> outputRotations = orientations.getRotations();
  vector<vector<double>> expectedQuats = {
    {-0.5, 0.5, 0.5, 0.5},
    {-0.5,-0.5,-0.5,-0.5},
    { 1.0, 0.0, 0.0, 0.0}
  };
  for (size_t i = 0; i < outputRotations.size(); i++) {
    vector<double> quats = outputRotations[i].toQuaternion();
    EXPECT_EQ(expectedQuats[i][0], quats[0]);
    EXPECT_EQ(expectedQuats[i][1], quats[1]);
    EXPECT_EQ(expectedQuats[i][2], quats[2]);
    EXPECT_EQ(expectedQuats[i][3], quats[3]);
  }
}

TEST_F(ConstOrientationTest, RotateAt) {
  Vec3d rotatedX = constRotation(orientations.rotateVectorAt(0.0, Vec3d(1.0, 0.0, 0.0)));
  Vec3d constRotatedX = constOrientations.rotateVectorAt(0.0, Vec3d(1.0, 0.0, 0.0));
  EXPECT_NEAR(rotatedX.x, constRotatedX.x, 1e-10);
  EXPECT_NEAR(rotatedX.y, constRotatedX.y, 1e-10);
  EXPECT_NEAR(rotatedX.z, constRotatedX.z, 1e-10);
  Vec3d rotatedY = constRotation(orientations.rotateVectorAt(0.0, Vec3d(0.0, 1.0, 0.0)));
  Vec3d constRotatedY = constOrientations.rotateVectorAt(0.0, Vec3d(0.0, 1.0, 0.0));
  EXPECT_NEAR(rotatedY.x, constRotatedY.x, 1e-10);
  EXPECT_NEAR(rotatedY.y, constRotatedY.y, 1e-10);
  EXPECT_NEAR(rotatedY.z, constRotatedY.z, 1e-10);
  Vec3d rotatedZ = constRotation(orientations.rotateVectorAt(0.0, Vec3d(0.0, 0.0, 1.0)));
  Vec3d constRotatedZ = constOrientations.rotateVectorAt(0.0, Vec3d(0.0, 0.0, 1.0));
  EXPECT_NEAR(rotatedZ.x, constRotatedZ.x, 1e-10);
  EXPECT_NEAR(rotatedZ.y, constRotatedZ.y, 1e-10);
  EXPECT_NEAR(rotatedZ.z, constRotatedZ.z, 1e-10);
}

TEST_F(ConstOrientationTest, OrientationMultiplication) {
  Orientations multOrientation = constOrientations * orientations;
  vector<Rotation> outputRotations = multOrientation.getRotations();
  vector<vector<double>> expectedQuats = {
    {-0.5, 0.5, 0.5, 0.5},
    {-0.5,-0.5,-0.5,-0.5},
    { 1.0, 0.0, 0.0, 0.0}
  };
  for (size_t i = 0; i < outputRotations.size(); i++) {
    vector<double> quats = outputRotations[i].toQuaternion();
    EXPECT_EQ(expectedQuats[i][0], quats[0]);
    EXPECT_EQ(expectedQuats[i][1], quats[1]);
    EXPECT_EQ(expectedQuats[i][2], quats[2]);
    EXPECT_EQ(expectedQuats[i][3], quats[3]);
  }
  vector<double> constQuats = multOrientation.getConstantRotation().toQuaternion();
  EXPECT_EQ(constQuats[0], 0);
  EXPECT_EQ(constQuats[1], 1);
  EXPECT_EQ(constQuats[2], 0);
  EXPECT_EQ(constQuats[3], 0);
}

TEST_F(ConstOrientationTest, OrientationInverse) {
  Orientations inverseOrientation = constOrientations.inverse();

  vector<int> constFrames = inverseOrientation.getConstantFrames();
  ASSERT_EQ(constFrames.size(), 0);

  vector<int> timeDepFrames = inverseOrientation.getTimeDependentFrames();
  ASSERT_EQ(timeDepFrames.size(), 5);
  EXPECT_EQ(timeDepFrames[0], 1);
  EXPECT_EQ(timeDepFrames[1], -74900);
  EXPECT_EQ(timeDepFrames[2], -74000);
  EXPECT_EQ(timeDepFrames[3], -74020);
  EXPECT_EQ(timeDepFrames[4], -74021);

  vector<double> newTimes = inverseOrientation.getTimes();
  ASSERT_EQ(newTimes.size(), 3);
  EXPECT_EQ(newTimes[0], 0.0);
  EXPECT_EQ(newTimes[1], 2.0);
  EXPECT_EQ(newTimes[2], 4.0);

  vector<Rotation> newRotations = inverseOrientation.getRotations();
  vector<vector<double>> expectedQuats = {
    {-0.5, -0.5, 0.5, -0.5},
    {-0.5, 0.5, 0.5, -0.5},
    {0.0, -1.0, 0.0, 0.0}
  };
  ASSERT_EQ(newRotations.size(), expectedQuats.size());
  for (size_t i = 0; i < newRotations.size(); i++) {
    vector<double> newQuats = newRotations[i].toQuaternion();
    ASSERT_EQ(newQuats.size(), 4) << "Rotation " << i;
    EXPECT_EQ(newQuats[0], expectedQuats[i][0]) << "Rotation " << i;
    EXPECT_EQ(newQuats[1], expectedQuats[i][1]) << "Rotation " << i;
    EXPECT_EQ(newQuats[2], expectedQuats[i][2]) << "Rotation " << i;
    EXPECT_EQ(newQuats[3], expectedQuats[i][3]) << "Rotation " << i;
  }

  vector<Vec3d> newAvs = inverseOrientation.getAngularVelocities();
  vector<Vec3d> expectedAvs = {
    Vec3d(-2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI),
    Vec3d(-2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI),
    Vec3d(-2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI, 2.0 / 3.0 * M_PI)
  };
  ASSERT_EQ(newAvs.size(), expectedAvs.size());
  EXPECT_EQ(newAvs[0].x, expectedAvs[0].x);
  EXPECT_EQ(newAvs[0].y, expectedAvs[0].y);
  EXPECT_EQ(newAvs[0].z, expectedAvs[0].z);
  EXPECT_EQ(newAvs[1].x, expectedAvs[1].x);
  EXPECT_EQ(newAvs[1].y, expectedAvs[1].y);
  EXPECT_EQ(newAvs[1].z, expectedAvs[1].z);
  EXPECT_EQ(newAvs[2].x, expectedAvs[2].x);
  EXPECT_EQ(newAvs[2].y, expectedAvs[2].y);
  EXPECT_EQ(newAvs[2].z, expectedAvs[2].z);
}
