#include "LocationModel.h"
#include <gtest/gtest.h>
#include <cmath>

using namespace location_correction;

// 测试LocationInfo类的基本功能
TEST(LocationModelTest, LocationInfoBasicTest) {
    // 创建一个LocationInfo对象
    LocationInfo location;
    location.latitude = 39.9042;
    location.longitude = 116.4074;
    location.accuracy = 5.0;
    location.altitude = 43.5;
    location.speed = 0.0;
    location.direction = 0.0;
    location.sourceType = LocationSource::GPS;
    location.timestamp = 1620000000000;
    
    // 验证属性值
    EXPECT_DOUBLE_EQ(location.latitude, 39.9042);
    EXPECT_DOUBLE_EQ(location.longitude, 116.4074);
    EXPECT_DOUBLE_EQ(location.accuracy, 5.0);
    EXPECT_DOUBLE_EQ(location.altitude, 43.5);
    EXPECT_DOUBLE_EQ(location.speed, 0.0);
    EXPECT_DOUBLE_EQ(location.direction, 0.0);
    EXPECT_EQ(location.sourceType, LocationSource::GPS);
    EXPECT_EQ(location.timestamp, 1620000000000);
}

// 测试LocationInfo的复制构造函数
TEST(LocationModelTest, LocationInfoCopyConstructorTest) {
    LocationInfo original;
    original.latitude = 39.9042;
    original.longitude = 116.4074;
    original.accuracy = 5.0;
    original.altitude = 43.5;
    original.sourceType = LocationSource::GPS;
    original.timestamp = 1620000000000;
    
    // 使用复制构造函数创建新对象
    LocationInfo copy = original;
    
    // 验证复制是否正确
    EXPECT_DOUBLE_EQ(copy.latitude, original.latitude);
    EXPECT_DOUBLE_EQ(copy.longitude, original.longitude);
    EXPECT_DOUBLE_EQ(copy.accuracy, original.accuracy);
    EXPECT_DOUBLE_EQ(copy.altitude, original.altitude);
    EXPECT_EQ(copy.sourceType, original.sourceType);
    EXPECT_EQ(copy.timestamp, original.timestamp);
}

// 测试LocationInfo的赋值运算符
TEST(LocationModelTest, LocationInfoAssignmentOperatorTest) {
    LocationInfo original;
    original.latitude = 39.9042;
    original.longitude = 116.4074;
    original.accuracy = 5.0;
    original.altitude = 43.5;
    original.sourceType = LocationSource::GPS;
    original.timestamp = 1620000000000;
    
    LocationInfo assigned;
    assigned = original;
    
    // 验证赋值是否正确
    EXPECT_DOUBLE_EQ(assigned.latitude, original.latitude);
    EXPECT_DOUBLE_EQ(assigned.longitude, original.longitude);
    EXPECT_DOUBLE_EQ(assigned.accuracy, original.accuracy);
    EXPECT_DOUBLE_EQ(assigned.altitude, original.altitude);
    EXPECT_EQ(assigned.sourceType, original.sourceType);
    EXPECT_EQ(assigned.timestamp, original.timestamp);
}

// 测试CorrectedLocation类的基本功能
TEST(LocationModelTest, CorrectedLocationBasicTest) {
    // 创建一个原始LocationInfo
    LocationInfo original;
    original.latitude = 39.9042;
    original.longitude = 116.4074;
    original.accuracy = 5.0;
    original.sourceType = LocationSource::GPS;
    
    // 创建一个CorrectedLocation
    CorrectedLocation corrected;
    corrected.originalLocation = original;
    corrected.latitude = 39.9043;
    corrected.longitude = 116.4075;
    corrected.accuracy = 2.0;
    corrected.correctionTime = 1620000001000;
    corrected.processed = true;
    
    // 验证属性值
    EXPECT_DOUBLE_EQ(corrected.originalLocation.latitude, 39.9042);
    EXPECT_DOUBLE_EQ(corrected.originalLocation.longitude, 116.4074);
    EXPECT_DOUBLE_EQ(corrected.latitude, 39.9043);
    EXPECT_DOUBLE_EQ(corrected.longitude, 116.4075);
    EXPECT_DOUBLE_EQ(corrected.accuracy, 2.0);
    EXPECT_EQ(corrected.correctionTime, 1620000001000);
    EXPECT_TRUE(corrected.processed);
}

// 测试CorrectedLocation的toLocationInfo方法
TEST(LocationModelTest, CorrectedLocationToLocationInfoTest) {
    CorrectedLocation corrected;
    corrected.latitude = 39.9043;
    corrected.longitude = 116.4075;
    corrected.accuracy = 2.0;
    corrected.altitude = 45.0;
    corrected.speed = 5.0;
    corrected.direction = 90.0;
    corrected.sourceType = LocationSource::GPS;
    corrected.timestamp = 1620000001000;
    
    // 转换为LocationInfo
    LocationInfo location = corrected.toLocationInfo();
    
    // 验证转换是否正确
    EXPECT_DOUBLE_EQ(location.latitude, 39.9043);
    EXPECT_DOUBLE_EQ(location.longitude, 116.4075);
    EXPECT_DOUBLE_EQ(location.accuracy, 2.0);
    EXPECT_DOUBLE_EQ(location.altitude, 45.0);
    EXPECT_DOUBLE_EQ(location.speed, 5.0);
    EXPECT_DOUBLE_EQ(location.direction, 90.0);
    EXPECT_EQ(location.sourceType, LocationSource::GPS);
    EXPECT_EQ(location.timestamp, 1620000001000);
}

// 测试ConfigModel的基本功能
TEST(LocationModelTest, ConfigModelTest) {
    // 测试SceneConfig
    SceneConfig sceneConfig;
    sceneConfig.sceneType = LocationScene::INDOOR;
    sceneConfig.maxSpeed = 5.0;
    sceneConfig.minAccuracy = 10.0;
    sceneConfig.weightForGPS = 0.3;
    sceneConfig.weightForWifi = 0.5;
    sceneConfig.weightForBaseStation = 0.2;
    
    EXPECT_EQ(sceneConfig.sceneType, LocationScene::INDOOR);
    EXPECT_DOUBLE_EQ(sceneConfig.maxSpeed, 5.0);
    EXPECT_DOUBLE_EQ(sceneConfig.minAccuracy, 10.0);
    EXPECT_DOUBLE_EQ(sceneConfig.weightForGPS, 0.3);
    EXPECT_DOUBLE_EQ(sceneConfig.weightForWifi, 0.5);
    EXPECT_DOUBLE_EQ(sceneConfig.weightForBaseStation, 0.2);
    
    // 测试AnomalyThresholds
    AnomalyThresholds thresholds;
    thresholds.maxTimeDifference = 5000;
    thresholds.maxDistanceDeviation = 100.0;
    thresholds.minAccuracyThreshold = 1.0;
    thresholds.maxAccuracyThreshold = 100.0;
    
    EXPECT_EQ(thresholds.maxTimeDifference, 5000);
    EXPECT_DOUBLE_EQ(thresholds.maxDistanceDeviation, 100.0);
    EXPECT_DOUBLE_EQ(thresholds.minAccuracyThreshold, 1.0);
    EXPECT_DOUBLE_EQ(thresholds.maxAccuracyThreshold, 100.0);
    
    // 测试CorrectionConfig
    CorrectionConfig correctionConfig;
    correctionConfig.minCorrectionInterval = 500;
    correctionConfig.enableAnomalyDetection = true;
    correctionConfig.enableDataFusion = true;
    correctionConfig.enableAdaptiveCorrection = true;
    correctionConfig.anomalyThresholds = thresholds;
    correctionConfig.sceneConfigs.push_back(sceneConfig);
    
    EXPECT_EQ(correctionConfig.minCorrectionInterval, 500);
    EXPECT_TRUE(correctionConfig.enableAnomalyDetection);
    EXPECT_TRUE(correctionConfig.enableDataFusion);
    EXPECT_TRUE(correctionConfig.enableAdaptiveCorrection);
    EXPECT_FALSE(correctionConfig.sceneConfigs.empty());
}