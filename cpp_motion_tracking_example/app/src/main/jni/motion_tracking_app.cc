/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tango_support_api.h>

#include <tango-gl/conversions.h>
#include "tango-motion-tracking/motion_tracking_app.h"

namespace tango_motion_tracking {
// The minimum Tango Core version required from this application.
constexpr int kTangoCoreMinimumVersion = 9377;
MotiongTrackingApp::MotiongTrackingApp() {}

MotiongTrackingApp::~MotiongTrackingApp() {
  if (tango_config_ != nullptr) {
    TangoConfig_free(tango_config_);
  }
}

void MotiongTrackingApp::OnCreate(JNIEnv* env, jobject activity) {
  // Check the installed version of the TangoCore.  If it is too old, then
  // it will not support the most up to date features.
  int version;
  TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);
  if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion) {
    LOGE("MotionTrackingApp::OnCreate, Tango Core version is out of date.");
    std::exit(EXIT_SUCCESS);
  }

  is_service_connected_ = false;
}

// Initialize Tango.
void MotiongTrackingApp::OnTangoServiceConnected(JNIEnv* env, jobject iBinder) {
  TangoErrorType ret = TangoService_setBinder(env, iBinder);
  if (ret != TANGO_SUCCESS) {
    LOGE(
        "MotiongTrackingApp: Failed to initialize Tango service with"
        "error code: %d",
        ret);
  }

  TangoSetupConfig();
  TangoConnect();

  is_service_connected_ = true;
}

int MotiongTrackingApp::TangoSetupConfig() {
  // Here, we'll configure the service to run in the way we'd want. For this
  // application, we'll start from the default configuration
  // (TANGO_CONFIG_DEFAULT). This enables basic motion tracking capabilities.
  tango_config_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
  if (tango_config_ == nullptr) {
    LOGE("MotiongTrackingApp: Failed to get default config form");
    return TANGO_ERROR;
  }

  // Set auto-recovery for motion tracking as requested by the user.
  int ret =
      TangoConfig_setBool(tango_config_, "config_enable_auto_recovery", true);
  if (ret != TANGO_SUCCESS) {
    LOGE(
        "MotiongTrackingApp: config_enable_auto_recovery() failed with error"
        "code: %d",
        ret);
    return ret;
  }

  return ret;
}

// Connect to Tango Service, service will start running, and
// pose can be queried.
bool MotiongTrackingApp::TangoConnect() {
  TangoErrorType ret = TangoService_connect(this, tango_config_);
  if (ret != TANGO_SUCCESS) {
    LOGE(
        "MotiongTrackingApp: Failed to connect to the Tango service with"
        "error code: %d",
        ret);
    return false;
  }
  return true;
}

void MotiongTrackingApp::OnPause() {
  TangoDisconnect();
  DeleteResources();
}

void MotiongTrackingApp::TangoDisconnect() {
  // When disconnecting from the Tango Service, it is important to make sure to
  // free your configuration object. Note that disconnecting from the service,
  // resets all configuration, and disconnects all callbacks. If an application
  // resumes after disconnecting, it must re-register configuration and
  // callbacks with the service.
  is_service_connected_ = false;
  TangoConfig_free(tango_config_);
  tango_config_ = nullptr;
  TangoService_disconnect();
}

void MotiongTrackingApp::InitializeGLContent(AAssetManager* aasset_manager) {
  TangoSupport_initialize(TangoService_getPoseAtTime);
  main_scene_.InitGLContent(aasset_manager);
}

void MotiongTrackingApp::SetViewPort(int width, int height) {
  main_scene_.SetupViewPort(width, height);
}

void MotiongTrackingApp::Render() {
  if (!is_service_connected_) {
    return;
  }

  TangoPoseData pose;

  TangoSupport_getPoseAtTime(
      0.0, TANGO_COORDINATE_FRAME_START_OF_SERVICE,
      TANGO_COORDINATE_FRAME_DEVICE, TANGO_SUPPORT_ENGINE_OPENGL,
      static_cast<TangoSupportDisplayRotation>(screen_rotation_), &pose);

  if (pose.status_code != TANGO_POSE_VALID) {
    LOGE("MotiongTrackingApp: Tango pose is not valid.");
    return;
  }

  // Rotate the logo cube related with the delta time
  main_scene_.RotateCubeByPose(pose);

  main_scene_.Render(pose);
}

void MotiongTrackingApp::DeleteResources() { main_scene_.DeleteResources(); }

void MotiongTrackingApp::SetScreenRotation(int screen_rotation) {
  screen_rotation_ = screen_rotation;
}

}  // namespace tango_motion_tracking
