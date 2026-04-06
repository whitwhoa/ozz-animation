////----------------------------------------------------------------------------//
////                                                                            //
//// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
//// and distributed under the MIT License (MIT).                               //
////                                                                            //
//// Copyright (c) Guillaume Blanc                                              //
////                                                                            //
//// Permission is hereby granted, free of charge, to any person obtaining a    //
//// copy of this software and associated documentation files (the "Software"), //
//// to deal in the Software without restriction, including without limitation  //
//// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
//// and/or sell copies of the Software, and to permit persons to whom the      //
//// Software is furnished to do so, subject to the following conditions:       //
////                                                                            //
//// The above copyright notice and this permission notice shall be included in //
//// all copies or substantial portions of the Software.                        //
////                                                                            //
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
//// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
//// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
//// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
//// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
//// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
//// DEALINGS IN THE SOFTWARE.                                                  //
////                                                                            //
////----------------------------------------------------------------------------//
//
//#include "framework/application.h"
//#include "framework/imgui.h"
//#include "framework/mesh.h"
//#include "framework/renderer.h"
//#include "framework/utils.h"
//#include "ozz/animation/runtime/animation.h"
//#include "ozz/animation/runtime/ik_aim_job.h"
//#include "ozz/animation/runtime/local_to_model_job.h"
//#include "ozz/animation/runtime/sampling_job.h"
//#include "ozz/animation/runtime/skeleton.h"
//#include "ozz/base/log.h"
//#include "ozz/base/maths/box.h"
//#include "ozz/base/maths/simd_math.h"
//#include "ozz/base/maths/simd_quaternion.h"
//#include "ozz/base/maths/soa_transform.h"
//#include "ozz/base/maths/vec_float.h"
//#include "ozz/options/options.h"
//
//// Skeleton archive can be specified as an option.
//OZZ_OPTIONS_DECLARE_STRING(skeleton,
//                           "Path to the skeleton (ozz archive format).",
//                           "media/skeleton.ozz", false)
//
//// Animation archive can be specified as an option.
//OZZ_OPTIONS_DECLARE_STRING(animation,
//                           "Path to the animation (ozz archive format).",
//                           "media/animation.ozz", false)
//
//// Mesh archive can be specified as an option.
//OZZ_OPTIONS_DECLARE_STRING(mesh,
//                           "Path to the skinned mesh (ozz archive format).",
//                           "media/mesh.ozz", false)
//
//// Defines IK chain joint names.
//// Joints must be from the same hierarchy (all ancestors of the first joint
//// listed) and ordered from child to parent.
//const char* kJointNames[] = {"Head", "Spine3", "Spine2", "Spine1"};
//const size_t kMaxChainLength = OZZ_ARRAY_SIZE(kJointNames);
//
//// Forward vector in head local-space.
//const ozz::math::SimdFloat4 kHeadForward = ozz::math::simd_float4::y_axis();
//
//// Defines Up vectors for each joint. This is skeleton/rig dependant.
//const ozz::math::SimdFloat4 kJointUpVectors[] = {
//    ozz::math::simd_float4::x_axis(), ozz::math::simd_float4::x_axis(),
//    ozz::math::simd_float4::x_axis(), ozz::math::simd_float4::x_axis()};
//static_assert(OZZ_ARRAY_SIZE(kJointUpVectors) == kMaxChainLength,
//              "Array size mismatch.");
//
//class LookAtSampleApplication : public ozz::sample::Application 
//{
// private:
//    // Playback animation controller. This is a utility class that helps with
//    // controlling animation playback time.
//    ozz::sample::PlaybackController controller_;
//
//    // Runtime skeleton.
//    ozz::animation::Skeleton skeleton_;
//
//    // Runtime animation.
//    ozz::animation::Animation animation_;
//
//    // Sampling context.
//    ozz::animation::SamplingJob::Context context_;
//
//    // Buffer of local transforms as sampled from animation_.
//    ozz::vector<ozz::math::SoaTransform> locals_;
//
//    // Buffer of model-space matrices.
//    ozz::vector<ozz::math::Float4x4> models_;
//
//    // Buffer of skinning matrices, result of the joint multiplication of the
//    // inverse rest pose with the model-space matrix.
//    ozz::vector<ozz::math::Float4x4> skinning_matrices_;
//
//    // The mesh used by the sample.
//    ozz::vector<ozz::sample::Mesh> meshes_;
//
//    // Indices of the joints that are IKed for look-at purpose.
//    // Joints must be from the same hierarchy (all ancestors of the first joint
//    // listed) and ordered from child to parent.
//    int joints_chain_[kMaxChainLength];
//
//    // Sample settings
//
//    // Target position management.
//    ozz::math::Float3 target_offset_ = {.2f, 1.5f, -.3f};
//    float target_extent_ = 1.f;
//    ozz::math::Float3 target_;
//
//    // Offset of the look at position in (head) joint local-space.
//    ozz::math::Float3 eyes_offset_ = {.07f, .1f, 0.f};
//
//    // IK settings
//
//    // Enable IK look at.
//    bool enable_ik_ = true;
//
//    // Set length of the chain that is IKed, between 0 and kMaxChainLength.
//    int chain_length_ = kMaxChainLength;
//
//    // Weight given to every joint of the chain. If any joint has a weight of 1,
//    // no other following joint will contribute (as the target will be reached).
//    float joint_weight_ = .5f;
//
//    // Overall weight given to the IK on the full chain. This allows blending in
//    // and out of IK.
//    float chain_weight_ = 1.f;
//
//    // Options
//    bool show_skin_ = true;
//    bool show_joints_ = false;
//    bool show_target_ = true;
//    bool show_eyes_offset_ = false;
//    bool show_forward_ = false;
//
// protected:
//
//    virtual bool OnInitialize() 
//    {
//      // Reading skeleton.
//      if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_))
//        return false;
//
//      // Look for each joint in the chain.
//      int found = 0;
//      for (int i = 0; i < skeleton_.num_joints() && found != kMaxChainLength; ++i) 
//      {
//        const char* joint_name = skeleton_.joint_names()[i];
//
//        if (std::strcmp(joint_name, kJointNames[found]) == 0) 
//        {
//          joints_chain_[found] = i;
//
//          // Restart search
//          ++found;
//          i = 0;
//        }
//      }
//
//      // Exit if all joints weren't found.
//      if (found != kMaxChainLength) 
//      {
//        ozz::log::Err() << "At least a joint wasn't found in the skeleton hierarchy." << std::endl;
//        return false;
//      }
//
//      // Validates joints are order from child to parent of the same hierarchy.
//      if (!ValidateJointsOrder(skeleton_, joints_chain_)) 
//      {
//        ozz::log::Err() << "Joints aren't properly ordered, they must be from "
//               "the same hierarchy (all ancestors of the first joint "
//               "listed) and ordered from child to parent."
//            << std::endl;
//        return false;
//      }
//
//      // Allocates runtime buffers.
//      const int num_soa_joints = skeleton_.num_soa_joints();
//      locals_.resize(num_soa_joints);
//      const int num_joints = skeleton_.num_joints();
//      models_.resize(num_joints);
//
//      // Allocates a context that matches animation requirements.
//      context_.Resize(num_joints);
//
//      // Reading animation.
//      if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_))
//        return false;
//
//      // Reading character mesh.
//      if (!ozz::sample::LoadMeshes(OPTIONS_mesh, &meshes_)) 
//        return false;
//      
//      // The number of joints of the mesh needs to match skeleton.
//      for (size_t m = 0; m < meshes_.size(); ++m) 
//      {
//        const ozz::sample::Mesh& mesh = meshes_[m];
//        if (num_joints < mesh.highest_joint_index()) 
//        {
//          ozz::log::Err() << "The provided mesh doesn't match skeleton (joint count mismatch)." << std::endl;
//          return false;
//        }
//      }
//      skinning_matrices_.resize(num_joints);
//
//      return true;
//    }
//
//  // Updates current animation time and skeleton pose.
//  virtual bool OnUpdate(float _dt, float _time) 
//  {
//    // Animates target position.
//    MoveTarget(_time);
//
//    // Updates current animation time.
//    controller_.Update(animation_, _dt);
//
//    // Samples optimized animation at t = animation_time_.
//    ozz::animation::SamplingJob sampling_job;
//    sampling_job.animation = &animation_;
//    sampling_job.context = &context_;
//    sampling_job.ratio = controller_.time_ratio();
//    sampling_job.output = make_span(locals_);
//
//    if (!sampling_job.Run())
//      return false;
//
//    // Converts from local-space to model-space matrices.
//    ozz::animation::LocalToModelJob ltm_job;
//    ltm_job.skeleton = &skeleton_;
//    ltm_job.input = make_span(locals_);
//    ltm_job.output = make_span(models_);
//    if (!ltm_job.Run())
//      return false;
//
//    // Early out if IK is disabled.
//    if (!enable_ik_)
//      return true;
//
//    // IK aim job setup.
//    ozz::animation::IKAimJob ik_job;
//
//    // Pole vector and target position are constant for the whole algorithm, in
//    // model-space.
//    ik_job.pole_vector = ozz::math::simd_float4::y_axis();
//    ik_job.target = ozz::math::simd_float4::Load3PtrU(&target_.x);
//
//    // The same quaternion will be used each time the job is run.
//    ozz::math::SimdQuaternion correction;
//    ik_job.joint_correction = &correction;
//
//    // The algorithm iteratively updates from the first joint (closer to the
//    // head) to the last (the further ancestor, closer to the pelvis). Joints
//    // order is already validated. For the first joint, aim IK is applied with
//    // the global forward and offset, so the forward vector aligns in direction
//    // of the target. If a weight lower that 1 is provided to the first joint,
//    // then it will not fully align to the target. In this case further joint
//    // will need to be updated. For the remaining joints, forward vector and
//    // offset position are computed in each joint local-space, before IK is
//    // applied:
//    // 1. Rotates forward and offset position based on the result of the
//    // previous joint IK.
//    // 2. Brings forward and offset back in joint local-space.
//    // Aim is iteratively applied up to the last selected joint of the
//    // hierarchy. A weight of 1 is given to the last joint so we can guarantee
//    // target is reached. Note that model-space transform of each joint doesn't
//    // need to be updated between each pass, as joints are ordered from child to
//    // parent.
//    int previous_joint = ozz::animation::Skeleton::kNoParent;
//    for (int i = 0, joint = joints_chain_[0]; i < chain_length_; ++i, previous_joint = joint, joint = joints_chain_[i]) 
//    {
//      // Setup the model-space matrix of the joint being processed by IK.
//      ik_job.joint = &models_[joint];
//
//      // Setup joint local-space up vector.
//      ik_job.up = kJointUpVectors[i];
//
//      // Setup weights of IK job.
//      // the last joint being processed needs a full weight (1.f) to ensure
//      // target is reached.
//      const bool last = i == chain_length_ - 1;
//      ik_job.weight = chain_weight_ * (last ? 1.f : joint_weight_);
//
//      // Setup offset and forward vector for the current joint being processed.
//      if (i == 0) 
//      {
//        // First joint, uses global forward and offset.
//        ik_job.offset = ozz::math::simd_float4::Load3PtrU(&eyes_offset_.x);
//        ik_job.forward = kHeadForward;
//      } 
//      else 
//      {
//        // Applies previous correction to "forward" and "offset", before
//        // bringing them to model-space (_ms).
//        const ozz::math::SimdFloat4 corrected_forward_ms = TransformVector(models_[previous_joint], TransformVector(correction, ik_job.forward));
//        const ozz::math::SimdFloat4 corrected_offset_ms = TransformPoint(models_[previous_joint], TransformVector(correction, ik_job.offset));
//
//        // Brings "forward" and "offset" to joint local-space
//        const ozz::math::Float4x4 inv_joint = Invert(models_[joint]);
//        ik_job.forward = TransformVector(inv_joint, corrected_forward_ms);
//        ik_job.offset = TransformPoint(inv_joint, corrected_offset_ms);
//      }
//
//      // Runs IK aim job.
//      if (!ik_job.Run())
//        return false;
//
//      // Apply IK quaternion to its respective local-space transforms.
//      ozz::sample::MultiplySoATransformQuaternion(joint, correction, make_span(locals_));
//    }
//
//    // Skeleton model-space matrices need to be updated again. This re-uses the
//    // already setup job, but limits the update to childs of the last joint (the
//    // parent-iest of the chain).
//    ltm_job.from = previous_joint;
//    if (!ltm_job.Run())
//      return false;
//
//    return true;
//  }
//
//  // Traverses the hierarchy from the first joint to the root, to check if
//  // joints are all ancestors (same branch), and ordered.
//  bool ValidateJointsOrder(const ozz::animation::Skeleton& _skeleton, ozz::span<const int> _joints) 
//  {
//    const size_t count = _joints.size();
//    if (count == 0)
//      return true;
//
//    size_t i = 1;
//    for (int joint = _joints[0], parent = _skeleton.joint_parents()[joint]; 
//        i != count && joint != ozz::animation::Skeleton::kNoParent; 
//        joint = parent, parent = _skeleton.joint_parents()[joint]) 
//    {
//      if (parent == _joints[i])
//        ++i;
//    }
//
//    return count == i;
//  }
//
//  // Sample arbitrary target animation implementation.
//  bool MoveTarget(float _time) 
//  {
//    const ozz::math::Float3 animated_target(std::sin(_time * .5f),
//                                            std::cos(_time * .25f),
//                                            std::cos(_time) * .5f + .5f);
//    target_ = target_offset_ + animated_target * target_extent_;
//    return true;
//  }
//
//  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) 
//  {
//    bool success = true;
//    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();
//    const float kAxeScale = .1f;
//    const ozz::math::Float4x4 kAxesScale =
//        ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(kAxeScale));
//
//    // Renders character.
//    if (show_skin_) {
//      // Builds skinning matrices.
//      // The mesh might not use (aka be skinned by) all skeleton joints. We
//      // use the joint remapping table (available from the mesh object) to
//      // reorder model-space matrices and build skinning ones.
//      for (size_t m = 0; m < meshes_.size(); ++m) {
//        const ozz::sample::Mesh& mesh = meshes_[m];
//        for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
//          skinning_matrices_[i] =
//              models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
//        }
//
//        success &= _renderer->DrawSkinnedMesh(
//            mesh, make_span(skinning_matrices_), identity);
//      }
//    } else {
//      // Renders skeleton only.
//      success &=
//          _renderer->DrawPosture(skeleton_, make_span(models_), identity);
//    }
//
//    // Showing joints
//    if (show_joints_) {
//      const float kSphereRadius = .02f;
//      for (int i = 0; i < chain_length_; ++i) {
//        const ozz::math::Float4x4& transform = models_[joints_chain_[i]];
//        success &= _renderer->DrawAxes(transform * kAxesScale);
//        success &= _renderer->DrawSphereIm(kSphereRadius, transform,
//                                           ozz::sample::kWhite);
//      }
//    }
//
//    // Showing target, as a box or axes depending on show_forward_ option.
//    if (show_target_) {
//      const auto target = ozz::math::Float4x4::Translation(target_);
//      if (show_forward_) {
//        success &= _renderer->DrawAxes(target * kAxesScale);
//      } else {
//        success &= _renderer->DrawSphereIm(.02f, target, ozz::sample::kGreen);
//      }
//    }
//
//    if (show_eyes_offset_ || show_forward_) {
//      const int head = joints_chain_[0];
//      const ozz::math::Float4x4 offset =
//          models_[head] * ozz::math::Float4x4::Translation(eyes_offset_);
//      if (show_eyes_offset_) {
//        success &= _renderer->DrawAxes(offset * kAxesScale);
//      }
//      if (show_forward_) {
//        ozz::math::Float3 forward;
//        ozz::math::Store3PtrU(kHeadForward, &forward.x);
//        ozz::math::Float3 line[2] = {{0, 0, 0}, forward * 10};
//        success &= _renderer->DrawLines(line, ozz::sample::kWhite, offset);
//      }
//    }
//    return success;
//  }
//
//  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
//    char label[64];
//
//    _im_gui->DoCheckBox("Enable ik", &enable_ik_);
//    snprintf(label, sizeof(label), "IK chain length: %d", chain_length_);
//    _im_gui->DoSlider(label, 0, kMaxChainLength, &chain_length_);
//    snprintf(label, sizeof(label), "Joint weight %.2g", joint_weight_);
//    _im_gui->DoSlider(label, 0.f, 1.f, &joint_weight_);
//    snprintf(label, sizeof(label), "Chain weight %.2g", chain_weight_);
//    _im_gui->DoSlider(label, 0.f, 1.f, &chain_weight_);
//
//    // Exposes animation runtime playback controls.
//    {
//      static bool open = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
//      if (open) {
//        controller_.OnGui(animation_, _im_gui);
//      }
//    }
//
//    {  // Target position
//      static bool opened = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target offset", &opened);
//      if (opened) {
//        const float kTargetRange = 3.f;
//
//        _im_gui->DoLabel("Animated extent");
//        snprintf(label, sizeof(label), "%.2g", target_extent_);
//        _im_gui->DoSlider(label, 0.f, kTargetRange, &target_extent_);
//
//        snprintf(label, sizeof(label), "x %.2g", target_offset_.x);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.x);
//        snprintf(label, sizeof(label), "y %.2g", target_offset_.y);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.y);
//        snprintf(label, sizeof(label), "z %.2g", target_offset_.z);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.z);
//      }
//    }
//
//    {  // Offset position
//      static bool opened = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Eyes offset", &opened);
//      if (opened) {
//        const float kOffsetRange = .5f;
//        snprintf(label, sizeof(label), "x %.2g", eyes_offset_.x);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &eyes_offset_.x);
//        snprintf(label, sizeof(label), "y %.2g", eyes_offset_.y);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &eyes_offset_.y);
//        snprintf(label, sizeof(label), "z %.2g", eyes_offset_.z);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &eyes_offset_.z);
//      }
//    }
//
//    // Options
//    {
//      _im_gui->DoCheckBox("Show skin", &show_skin_);
//      _im_gui->DoCheckBox("Show joints", &show_joints_);
//      _im_gui->DoCheckBox("Show target", &show_target_);
//      _im_gui->DoCheckBox("Show eyes offset", &show_eyes_offset_);
//      _im_gui->DoCheckBox("Show forward", &show_forward_);
//    }
//
//    return true;
//  }
//
//  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
//    const ozz::math::Float3 radius(target_extent_ * .8f);
//    _bound->min = target_offset_ - radius;
//    _bound->max = target_offset_ + radius;
//  }
//
//
//};
//
//int main(int _argc, const char** _argv) {
//  const char* title = "Ozz-animation sample: Look at";
//  return LookAtSampleApplication().Run(_argc, _argv, "1.0", title);
//}













////----------------------------------------------------------------------------//
////                                                                            //
//// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
//// and distributed under the MIT License (MIT).                               //
////                                                                            //
//// Copyright (c) Guillaume Blanc                                              //
////                                                                            //
//// Permission is hereby granted, free of charge, to any person obtaining a    //
//// copy of this software and associated documentation files (the "Software"), //
//// to deal in the Software without restriction, including without limitation  //
//// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
//// and/or sell copies of the Software, and to permit persons to whom the      //
//// Software is furnished to do so, subject to the following conditions:       //
////                                                                            //
//// The above copyright notice and this permission notice shall be included in //
//// all copies or substantial portions of the Software.                        //
////                                                                            //
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
//// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
//// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
//// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
//// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
//// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
//// DEALINGS IN THE SOFTWARE.                                                  //
////                                                                            //
////----------------------------------------------------------------------------//
//
//#include "framework/application.h"
//#include "framework/imgui.h"
//#include "framework/mesh.h"
//#include "framework/renderer.h"
//#include "framework/utils.h"
//#include "ozz/animation/runtime/animation.h"
//#include "ozz/animation/runtime/ik_aim_job.h"
//#include "ozz/animation/runtime/local_to_model_job.h"
//#include "ozz/animation/runtime/sampling_job.h"
//#include "ozz/animation/runtime/skeleton.h"
//#include "ozz/base/log.h"
//#include "ozz/base/maths/box.h"
//#include "ozz/base/maths/simd_math.h"
//#include "ozz/base/maths/simd_quaternion.h"
//#include "ozz/base/maths/soa_transform.h"
//#include "ozz/base/maths/vec_float.h"
//#include "ozz/options/options.h"
//
//// Skeleton archive can be specified as an option.
//OZZ_OPTIONS_DECLARE_STRING(skeleton,
//                           "Path to the skeleton (ozz archive format).",
//                           "media/ct1.ozz", false)
//
//// Animation archive can be specified as an option.
//OZZ_OPTIONS_DECLARE_STRING(animation,
//                           "Path to the animation (ozz archive format).",
//                           "media/pistol_idle.ozz", false)
//
//
//// Defines IK chain joint names.
//// Joints must be from the same hierarchy (all ancestors of the first joint
//// listed) and ordered from child to parent.
//const char* kJointNames[] = {
//    //"mixamorig:Head", 
//    //"mixamorig:Neck",
//    "mixamorig:Spine2", 
//    "mixamorig:Spine1", 
//    "mixamorig:Spine"
//};
//const size_t kMaxChainLength = OZZ_ARRAY_SIZE(kJointNames);
//
//// Forward vector in Joint local-space.
//const ozz::math::SimdFloat4 kAimForward = ozz::math::simd_float4::z_axis();
//
//// Defines Up vectors for each joint. This is skeleton/rig dependant.
//const ozz::math::SimdFloat4 kJointUpVectors[] = {
//    ozz::math::simd_float4::y_axis(), ozz::math::simd_float4::y_axis(),
//    ozz::math::simd_float4::y_axis()};
//static_assert(OZZ_ARRAY_SIZE(kJointUpVectors) == kMaxChainLength, "Array size mismatch.");
//
////const float kPerJointWeights[] = {0.10f, 0.20f, 0.45f, 0.70f, 0.90f};
////const float kPerJointWeights[] = {1.f, 0.33f, 0.33f};
//const float kPerJointWeights[] = {0.25f, 0.35f, 1.0f};
//static_assert(OZZ_ARRAY_SIZE(kPerJointWeights) == kMaxChainLength, "Array size mismatch.");
//
//class LookAtSampleApplication : public ozz::sample::Application {
// private:
//  // Playback animation controller. This is a utility class that helps with
//  // controlling animation playback time.
//  ozz::sample::PlaybackController controller_;
//
//  // Runtime skeleton.
//  ozz::animation::Skeleton skeleton_;
//
//  // Runtime animation.
//  ozz::animation::Animation animation_;
//
//  // Sampling context.
//  ozz::animation::SamplingJob::Context context_;
//
//  // Buffer of local transforms as sampled from animation_.
//  ozz::vector<ozz::math::SoaTransform> locals_;
//
//  // Buffer of model-space matrices.
//  ozz::vector<ozz::math::Float4x4> models_;
//
//
//  // Indices of the joints that are IKed for look-at purpose.
//  // Joints must be from the same hierarchy (all ancestors of the first joint
//  // listed) and ordered from child to parent.
//  int joints_chain_[kMaxChainLength];
//
//  // Sample settings
//
//  // Target position management.
//  //ozz::math::Float3 target_offset_ = {.2f, 1.5f, -.3f};
//  ozz::math::Float3 target_offset_ = {0.f, 0.f, 0.f};
//  float target_extent_ = 1.f;
//  ozz::math::Float3 target_;
//
//  // Offset of the look at position in (head) joint local-space.
//  //ozz::math::Float3 aim_offset_ = {.07f, .1f, 0.f};
//  ozz::math::Float3 aim_offset_ = {0.f, 0.f, 0.f};
//  //ozz::math::Float3 aim_offset_ = {0.32f, 0.29f, 0.033f};
//  //ozz::math::Float3 aim_offset_ = {0.71f, 0.24f, 0.f};
//
//  // IK settings
//
//  // Enable IK look at.
//  bool enable_ik_ = true;
//
//  // Set length of the chain that is IKed, between 0 and kMaxChainLength.
//  int chain_length_ = kMaxChainLength;
//
//  // Weight given to every joint of the chain. If any joint has a weight of 1,
//  // no other following joint will contribute (as the target will be reached).
//  float joint_weight_ = .5f;
//
//  // Overall weight given to the IK on the full chain. This allows blending in
//  // and out of IK.
//  float chain_weight_ = 1.f;
//
//  // Options
//  bool show_skin_ = false;
//  bool show_joints_ = false;
//  bool show_target_ = true;
//  bool show_aim_offset_ = false;
//  bool show_forward_ = false;
//
// protected:
//  virtual bool OnInitialize() {
//    // Reading skeleton.
//    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) return false;
//
//    // Look for each joint in the chain.
//    int found = 0;
//    for (int i = 0; i < skeleton_.num_joints() && found != kMaxChainLength;
//         ++i) {
//      const char* joint_name = skeleton_.joint_names()[i];
//
//      if (std::strcmp(joint_name, kJointNames[found]) == 0) {
//        joints_chain_[found] = i;
//
//        // Restart search
//        ++found;
//        i = 0;
//      }
//    }
//
//    // Exit if all joints weren't found.
//    if (found != kMaxChainLength) {
//      ozz::log::Err()
//          << "At least a joint wasn't found in the skeleton hierarchy."
//          << std::endl;
//      return false;
//    }
//
//    // Validates joints are order from child to parent of the same hierarchy.
//    if (!ValidateJointsOrder(skeleton_, joints_chain_)) {
//      ozz::log::Err() << "Joints aren't properly ordered, they must be from "
//                         "the same hierarchy (all ancestors of the first joint "
//                         "listed) and ordered from child to parent."
//                      << std::endl;
//      return false;
//    }
//
//    // Allocates runtime buffers.
//    const int num_soa_joints = skeleton_.num_soa_joints();
//    locals_.resize(num_soa_joints);
//    const int num_joints = skeleton_.num_joints();
//    models_.resize(num_joints);
//
//    // Allocates a context that matches animation requirements.
//    context_.Resize(num_joints);
//
//    // Reading animation.
//    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_))
//      return false;
//
//    return true;
//  }
//
//  // Updates current animation time and skeleton pose.
//  virtual bool OnUpdate(float _dt, float _time) {
//    // Animates target position.
//    MoveTarget(_time);
//
//    // Updates current animation time.
//    controller_.Update(animation_, _dt);
//
//    // Samples optimized animation at t = animation_time_.
//    ozz::animation::SamplingJob sampling_job;
//    sampling_job.animation = &animation_;
//    sampling_job.context = &context_;
//    sampling_job.ratio = controller_.time_ratio();
//    sampling_job.output = make_span(locals_);
//
//    if (!sampling_job.Run()) return false;
//
//    // Converts from local-space to model-space matrices.
//    ozz::animation::LocalToModelJob ltm_job;
//    ltm_job.skeleton = &skeleton_;
//    ltm_job.input = make_span(locals_);
//    ltm_job.output = make_span(models_);
//    if (!ltm_job.Run()) return false;
//
//    // Early out if IK is disabled.
//    if (!enable_ik_) return true;
//
//    // IK aim job setup.
//    ozz::animation::IKAimJob ik_job;
//
//    // Pole vector and target position are constant for the whole algorithm, in
//    // model-space.
//    ik_job.pole_vector = ozz::math::simd_float4::y_axis();
//    ik_job.target = ozz::math::simd_float4::Load3PtrU(&target_.x);
//
//    // The same quaternion will be used each time the job is run.
//    ozz::math::SimdQuaternion correction;
//    ik_job.joint_correction = &correction;
//
//    // The algorithm iteratively updates from the first joint (closer to the
//    // head) to the last (the further ancestor, closer to the pelvis). Joints
//    // order is already validated. For the first joint, aim IK is applied with
//    // the global forward and offset, so the forward vector aligns in direction
//    // of the target. If a weight lower that 1 is provided to the first joint,
//    // then it will not fully align to the target. In this case further joint
//    // will need to be updated. For the remaining joints, forward vector and
//    // offset position are computed in each joint local-space, before IK is
//    // applied:
//    // 1. Rotates forward and offset position based on the result of the
//    // previous joint IK.
//    // 2. Brings forward and offset back in joint local-space.
//    // Aim is iteratively applied up to the last selected joint of the
//    // hierarchy. A weight of 1 is given to the last joint so we can guarantee
//    // target is reached. Note that model-space transform of each joint doesn't
//    // need to be updated between each pass, as joints are ordered from child to
//    // parent.
//    int previous_joint = ozz::animation::Skeleton::kNoParent;
//    for (int i = 0, joint = joints_chain_[0]; i < chain_length_; ++i, previous_joint = joint, joint = joints_chain_[i]) 
//    {
//      // Setup the model-space matrix of the joint being processed by IK.
//      ik_job.joint = &models_[joint];
//
//      // Setup joint local-space up vector.
//      ik_job.up = kJointUpVectors[i];
//
//      // Setup weights of IK job.
//      // the last joint being processed needs a full weight (1.f) to ensure
//      // target is reached.
//      //const bool last = i == chain_length_ - 1;
//      //ik_job.weight = chain_weight_ * (last ? 1.f : joint_weight_);
//      ik_job.weight = chain_weight_ * kPerJointWeights[i];
//
//      // Setup offset and forward vector for the current joint being processed.
//      if (i == 0) 
//      {
//        // First joint, uses global forward and offset.
//        ik_job.offset = ozz::math::simd_float4::Load3PtrU(&aim_offset_.x);
//        ik_job.forward = kAimForward;
//      } 
//      else 
//      {
//        // Applies previous correction to "forward" and "offset", before
//        // bringing them to model-space (_ms).
//        const ozz::math::SimdFloat4 corrected_forward_ms = TransformVector(models_[previous_joint], TransformVector(correction, ik_job.forward));
//        const ozz::math::SimdFloat4 corrected_offset_ms = TransformPoint(models_[previous_joint], TransformVector(correction, ik_job.offset));
//
//        // Brings "forward" and "offset" to joint local-space
//        const ozz::math::Float4x4 inv_joint = Invert(models_[joint]);
//        ik_job.forward = TransformVector(inv_joint, corrected_forward_ms);
//        ik_job.offset = TransformPoint(inv_joint, corrected_offset_ms);
//      }
//
//      // Runs IK aim job.
//      if (!ik_job.Run()) 
//          return false;
//
//      // Apply IK quaternion to its respective local-space transforms.
//      ozz::sample::MultiplySoATransformQuaternion(joint, correction, make_span(locals_));
//    }
//
//    // Skeleton model-space matrices need to be updated again. This re-uses the
//    // already setup job, but limits the update to childs of the last joint (the
//    // parent-iest of the chain).
//    ltm_job.from = previous_joint;
//    if (!ltm_job.Run()) return false;
//
//    return true;
//  }
//
//  // Traverses the hierarchy from the first joint to the root, to check if
//  // joints are all ancestors (same branch), and ordered.
//  bool ValidateJointsOrder(const ozz::animation::Skeleton& _skeleton,
//                           ozz::span<const int> _joints) {
//    const size_t count = _joints.size();
//    if (count == 0) return true;
//
//    size_t i = 1;
//    for (int joint = _joints[0], parent = _skeleton.joint_parents()[joint];
//         i != count && joint != ozz::animation::Skeleton::kNoParent;
//         joint = parent, parent = _skeleton.joint_parents()[joint]) {
//      if (parent == _joints[i]) ++i;
//    }
//
//    return count == i;
//  }
//
//  // Sample arbitrary target animation implementation.
//  //bool MoveTarget(float _time) 
//  bool MoveTarget(float _time) 
//  {
//    //const ozz::math::Float3 animated_target(std::sin(_time * .5f),
//    //                                        std::cos(_time * .25f),
//    //                                        std::cos(_time) * .5f + .5f);
//
//      const ozz::math::Float3 animated_target(0.f,
//                                            1.f + (std::cos(_time * 0.5f) * 4.f),
//                                            2.f);
//
//   //const ozz::math::Float3 animated_target = {0.f, 6.f, 2.f};
//
//    target_ = target_offset_ + animated_target * target_extent_;
//    //target_ = animated_target * target_extent_;
//
//    return true;
//  }
//
//  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
//    bool success = true;
//    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();
//    const float kAxeScale = .1f;
//    const ozz::math::Float4x4 kAxesScale =
//        ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(kAxeScale));
//
//
//      // Renders skeleton only.
//      success &=
//          _renderer->DrawPosture(skeleton_, make_span(models_), identity);
//
//    // Showing joints
//    if (show_joints_) {
//      const float kSphereRadius = .02f;
//      for (int i = 0; i < chain_length_; ++i) {
//        const ozz::math::Float4x4& transform = models_[joints_chain_[i]];
//        success &= _renderer->DrawAxes(transform * kAxesScale);
//        success &= _renderer->DrawSphereIm(kSphereRadius, transform,
//                                           ozz::sample::kWhite);
//      }
//    }
//
//    // Showing target, as a box or axes depending on show_forward_ option.
//    if (show_target_) {
//      const auto target = ozz::math::Float4x4::Translation(target_);
//      if (show_forward_) {
//        success &= _renderer->DrawAxes(target * kAxesScale);
//      } else {
//        success &= _renderer->DrawSphereIm(.02f, target, ozz::sample::kGreen);
//      }
//    }
//
//    if (show_aim_offset_ || show_forward_) 
//    {
//      const int first_joint = joints_chain_[0];
//      const ozz::math::Float4x4 offset = models_[first_joint] * ozz::math::Float4x4::Translation(aim_offset_);
//      if (show_aim_offset_)
//      {
//        success &= _renderer->DrawAxes(offset * kAxesScale);
//      }
//      if (show_forward_) 
//      {
//        ozz::math::Float3 forward;
//        ozz::math::Store3PtrU(kAimForward, &forward.x);
//        ozz::math::Float3 line[2] = {{0, 0, 0}, forward * 10};
//        success &= _renderer->DrawLines(line, ozz::sample::kWhite, offset);
//      }
//    }
//    return success;
//  }
//
//  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
//    char label[64];
//
//    _im_gui->DoCheckBox("Enable ik", &enable_ik_);
//    snprintf(label, sizeof(label), "IK chain length: %d", chain_length_);
//    _im_gui->DoSlider(label, 0, kMaxChainLength, &chain_length_);
//    snprintf(label, sizeof(label), "Joint weight %.2g", joint_weight_);
//    _im_gui->DoSlider(label, 0.f, 1.f, &joint_weight_);
//    snprintf(label, sizeof(label), "Chain weight %.2g", chain_weight_);
//    _im_gui->DoSlider(label, 0.f, 1.f, &chain_weight_);
//
//    // Exposes animation runtime playback controls.
//    {
//      static bool open = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
//      if (open) {
//        controller_.OnGui(animation_, _im_gui);
//      }
//    }
//
//    {  // Target position
//      static bool opened = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target offset", &opened);
//      if (opened) {
//        const float kTargetRange = 3.f;
//
//        _im_gui->DoLabel("Animated extent");
//        snprintf(label, sizeof(label), "%.2g", target_extent_);
//        _im_gui->DoSlider(label, 0.f, kTargetRange, &target_extent_);
//
//        snprintf(label, sizeof(label), "x %.2g", target_offset_.x);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.x);
//        snprintf(label, sizeof(label), "y %.2g", target_offset_.y);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.y);
//        snprintf(label, sizeof(label), "z %.2g", target_offset_.z);
//        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
//                          &target_offset_.z);
//      }
//    }
//
//    {  // Offset position
//      static bool opened = true;
//      ozz::sample::ImGui::OpenClose oc(_im_gui, "Aim offset", &opened);
//      if (opened) {
//        const float kOffsetRange = 2.5f;
//        snprintf(label, sizeof(label), "x %.2g", aim_offset_.x);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.x);
//        snprintf(label, sizeof(label), "y %.2g", aim_offset_.y);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.y);
//        snprintf(label, sizeof(label), "z %.2g", aim_offset_.z);
//        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.z);
//      }
//    }
//
//    // Options
//    {
//      _im_gui->DoCheckBox("Show skin", &show_skin_);
//      _im_gui->DoCheckBox("Show joints", &show_joints_);
//      _im_gui->DoCheckBox("Show target", &show_target_);
//      _im_gui->DoCheckBox("Show aim offset", &show_aim_offset_);
//      _im_gui->DoCheckBox("Show forward", &show_forward_);
//    }
//
//    return true;
//  }
//
//  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
//    const ozz::math::Float3 radius(target_extent_ * .8f);
//    _bound->min = target_offset_ - radius;
//    _bound->max = target_offset_ + radius;
//  }
//};
//
//int main(int _argc, const char** _argv) {
//  const char* title = "Ozz-animation sample: Look at";
//  return LookAtSampleApplication().Run(_argc, _argv, "1.0", title);
//}










/*
* THIS IS THE WORKING LOOK AT EXAMPLE. Commented out to use this project to hack around
* at trying to do the bone pitch rotation without all the ik logic
* 
#include <cstring>
#include <cmath>

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/ik_aim_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/ct1.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/pistol_idle.ozz", false)

// Defines IK chain joint names.
// Joints must be from the same hierarchy (all ancestors of the first joint
// listed) and ordered from child to parent.
const char* kJointNames[] = {"mixamorig:Spine2", "mixamorig:Spine1", "mixamorig:Spine"};
const size_t kMaxChainLength = OZZ_ARRAY_SIZE(kJointNames);

// Target "gameplay" frame in model/world space used for calibration.
// These define what "forward" and "up" mean in your neutral aim pose.
const ozz::math::SimdFloat4 kModelAimForward = ozz::math::simd_float4::z_axis();
const ozz::math::SimdFloat4 kModelAimUp = ozz::math::simd_float4::y_axis();

// Distributed chain weights.
// Last joint is 1.0f so the chain can still converge.
const float kPerJointWeights[] = {0.25f, 0.35f, 1.0f};
static_assert(OZZ_ARRAY_SIZE(kPerJointWeights) == kMaxChainLength, "Array size mismatch.");

class LookAtSampleApplication : public ozz::sample::Application 
{
 private:
  // Playback animation controller.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model-space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

  // Indices of the joints that are IKed for look-at purpose.
  int joints_chain_[kMaxChainLength];

  // Per-joint calibrated local aiming basis.
  ozz::math::SimdFloat4 joint_aim_forward_[kMaxChainLength];
  ozz::math::SimdFloat4 joint_aim_up_[kMaxChainLength];

  // Target position management.
  ozz::math::Float3 target_offset_ = {0.f, 0.f, 0.f};
  float target_extent_ = 1.f;
  ozz::math::Float3 target_;

  // Offset of the look-at position in first joint local-space.
  ozz::math::Float3 aim_offset_ = {0.f, 0.f, 0.f};

  // IK settings.
  bool enable_ik_ = true;
  int chain_length_ = static_cast<int>(kMaxChainLength);
  float chain_weight_ = 1.f;

  // Options.
  bool show_skin_ = false;
  bool show_joints_ = false;
  bool show_target_ = true;
  bool show_aim_offset_ = false;
  bool show_forward_ = false;
  bool show_calibrated_basis_ = true;

 private:
  bool ValidateJointsOrder(const ozz::animation::Skeleton& _skeleton, ozz::span<const int> _joints) 
  {
    const size_t count = _joints.size();

    if (count == 0)
      return true;


    size_t i = 1;
    for (int joint = _joints[0], parent = _skeleton.joint_parents()[joint];
         i != count && joint != ozz::animation::Skeleton::kNoParent;
         joint = parent, parent = _skeleton.joint_parents()[joint]) 
    {
      if (parent == _joints[i]) 
        ++i;
    }

    return count == i;
  }

  bool MoveTarget(float _time) 
  {
    const ozz::math::Float3 animated_target(0.f, 1.f + (std::cos(_time * 0.5f) * 4.f), 2.f);
    target_ = target_offset_ + animated_target * target_extent_;
    return true;
  }

  // Samples the loaded animation at ratio 0.0 and computes, for each spine
  // joint, the local-space forward/up vectors that correspond to the desired
  // model-space aim frame.
  bool CalibrateAimBasisFromReferencePose() 
  {
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    ozz::animation::SamplingJob::Context calib_context;
    calib_context.Resize(num_joints);

    ozz::vector<ozz::math::SoaTransform> calib_locals;
    calib_locals.resize(num_soa_joints);

    ozz::vector<ozz::math::Float4x4> calib_models;
    calib_models.resize(num_joints);

    // Sample the reference animation at ratio 0.0.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.context = &calib_context;
    sampling_job.ratio = 0.f;
    sampling_job.output = make_span(calib_locals);

    if (!sampling_job.Run()) 
    {
      ozz::log::Err() << "Failed to sample calibration pose." << std::endl;
      return false;
    }

    // Build model-space transforms for the calibration pose.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(calib_locals);
    ltm_job.output = make_span(calib_models);

    if (!ltm_job.Run()) 
    {
      ozz::log::Err() << "Failed to build calibration model transforms." << std::endl;
      return false;
    }

    // For each joint, convert desired model/world forward/up into that joint's
    // local space at the calibration pose.
    for (size_t i = 0; i < kMaxChainLength; ++i) 
    {
      const int joint = joints_chain_[i];
      const ozz::math::Float4x4 inv_joint = Invert(calib_models[joint]);

      joint_aim_forward_[i] = TransformVector(inv_joint, kModelAimForward);
      joint_aim_up_[i] = TransformVector(inv_joint, kModelAimUp);
    }

    return true;
  }

 protected:
  virtual bool OnInitialize() 
  {
    // Reading skeleton.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) 
      return false;

    // Look for each joint in the chain.
    int found = 0;
    for (int i = 0; i < skeleton_.num_joints() && found != kMaxChainLength; ++i) 
    {
      const char* joint_name = skeleton_.joint_names()[i];

      if (std::strcmp(joint_name, kJointNames[found]) == 0) 
      {
        joints_chain_[found] = i;

        // Restart search.
        ++found;
        i = 0;
      }
    }

    // Exit if all joints weren't found.
    if (found != static_cast<int>(kMaxChainLength)) 
    {
      ozz::log::Err() << "At least a joint wasn't found in the skeleton hierarchy." << std::endl;
      return false;
    }

    // Validates joints are ordered child -> parent on the same branch.
    if (!ValidateJointsOrder(skeleton_, joints_chain_)) 
    {
      ozz::log::Err() << "Joints aren't properly ordered. They must be from the same hierarchy and ordered child to parent." << std::endl;
      return false;
    }

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);

    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a context that matches animation requirements.
    context_.Resize(num_joints);

    // Reading animation.
    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_)) 
      return false;

    // Calibrate local aim basis from the loaded animation at ratio 0.0.
    if (!CalibrateAimBasisFromReferencePose()) 
      return false;

    return true;
  }

  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float _time) 
  {
    // Animates target position.
    MoveTarget(_time);

    // Updates current animation time.
    controller_.Update(animation_, _dt);

    // Samples optimized animation at t = animation_time_.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.context = &context_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_span(locals_);

    if (!sampling_job.Run())
      return false;

    // Converts from local-space to model-space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);

    if (!ltm_job.Run())
      return false;

    // Early out if IK is disabled.
    if (!enable_ik_)
      return true;

    // IK aim job setup.
    ozz::animation::IKAimJob ik_job;

    // Pole vector and target position are constant for the whole algorithm, in model-space.
    ik_job.pole_vector = kModelAimUp;
    ik_job.target = ozz::math::simd_float4::Load3PtrU(&target_.x);

    // The same quaternion will be used each time the job is run.
    ozz::math::SimdQuaternion correction;
    ik_job.joint_correction = &correction;

    // Iteratively update from child -> parent.
    int previous_joint = ozz::animation::Skeleton::kNoParent;
    for (int i = 0, joint = joints_chain_[0]; i < chain_length_; ++i, previous_joint = joint, joint = joints_chain_[i]) 
    {
      // Setup the model-space matrix of the joint being processed by IK.
      ik_job.joint = &models_[joint];

      // Use calibrated local up vector for this joint.
      ik_job.up = joint_aim_up_[i];

      // Setup weights of IK job.
      ik_job.weight = chain_weight_ * kPerJointWeights[i];

      // Setup offset and forward vector for the current joint being processed.
      if (i == 0) 
      {
        // First joint uses calibrated forward and authored offset.
        ik_job.offset = ozz::math::simd_float4::Load3PtrU(&aim_offset_.x);
        ik_job.forward = joint_aim_forward_[i];
      } 
      else 
      {
        // Apply previous correction to forward/offset, bring back to current joint local-space.
        const ozz::math::SimdFloat4 corrected_forward_ms = TransformVector(models_[previous_joint], TransformVector(correction, ik_job.forward));
        const ozz::math::SimdFloat4 corrected_offset_ms = TransformPoint(models_[previous_joint], TransformVector(correction, ik_job.offset));

        const ozz::math::Float4x4 inv_joint = Invert(models_[joint]);
        ik_job.forward = TransformVector(inv_joint, corrected_forward_ms);
        ik_job.offset = TransformPoint(inv_joint, corrected_offset_ms);
      }

      // Runs IK aim job.
      if (!ik_job.Run()) 
          return false;

      // Apply IK quaternion to its respective local-space transform.
      ozz::sample::MultiplySoATransformQuaternion(joint, correction, make_span(locals_));
    }

    // Skeleton model-space matrices need to be updated again.
    ltm_job.from = previous_joint;
    if (!ltm_job.Run())
      return false;


    return true;
  }

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {
    bool success = true;

    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();
    const float kAxeScale = .1f;
    const ozz::math::Float4x4 kAxesScale =
        ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(kAxeScale));

    // Renders skeleton only.
    success &= _renderer->DrawPosture(skeleton_, make_span(models_), identity);

    // Show joints.
    if (show_joints_) {
      const float kSphereRadius = .02f;
      for (int i = 0; i < chain_length_; ++i) {
        const ozz::math::Float4x4& transform = models_[joints_chain_[i]];
        success &= _renderer->DrawAxes(transform * kAxesScale);
        success &= _renderer->DrawSphereIm(kSphereRadius, transform,
                                           ozz::sample::kWhite);
      }
    }

    // Show target.
    if (show_target_) {
      const auto target = ozz::math::Float4x4::Translation(target_);
      if (show_forward_) {
        success &= _renderer->DrawAxes(target * kAxesScale);
      } else {
        success &= _renderer->DrawSphereIm(.02f, target, ozz::sample::kGreen);
      }
    }

    // Show first-joint offset and calibrated forward.
    if (show_aim_offset_ || show_forward_) {
      const int first_joint = joints_chain_[0];
      const ozz::math::Float4x4 offset =
          models_[first_joint] * ozz::math::Float4x4::Translation(aim_offset_);

      if (show_aim_offset_) {
        success &= _renderer->DrawAxes(offset * kAxesScale);
      }

      if (show_forward_) {
        ozz::math::Float3 forward;
        ozz::math::Store3PtrU(joint_aim_forward_[0], &forward.x);
        ozz::math::Float3 line[2] = {{0, 0, 0}, forward * 10.f};
        success &= _renderer->DrawLines(line, ozz::sample::kWhite, offset);
      }
    }

    // Optional debug: show calibrated basis at each chain joint.
    if (show_calibrated_basis_) {
      for (int i = 0; i < chain_length_; ++i) {
        const ozz::math::Float4x4& joint_tm = models_[joints_chain_[i]];

        ozz::math::Float3 fwd;
        ozz::math::Float3 up;
        ozz::math::Store3PtrU(joint_aim_forward_[i], &fwd.x);
        ozz::math::Store3PtrU(joint_aim_up_[i], &up.x);

        {
          ozz::math::Float3 line[2] = {{0, 0, 0}, fwd * 0.35f};
          success &= _renderer->DrawLines(line, ozz::sample::kWhite, joint_tm);
        }
        {
          ozz::math::Float3 line[2] = {{0, 0, 0}, up * 0.25f};
          success &= _renderer->DrawLines(line, ozz::sample::kGreen, joint_tm);
        }
      }
    }

    return success;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    char label[64];

    _im_gui->DoCheckBox("Enable ik", &enable_ik_);
    snprintf(label, sizeof(label), "IK chain length: %d", chain_length_);
    _im_gui->DoSlider(label, 0, static_cast<int>(kMaxChainLength),
                      &chain_length_);
    snprintf(label, sizeof(label), "Chain weight %.2g", chain_weight_);
    _im_gui->DoSlider(label, 0.f, 1.f, &chain_weight_);

    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(animation_, _im_gui);
      }
    }

    {  // Target position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Target offset", &opened);
      if (opened) {
        const float kTargetRange = 3.f;

        _im_gui->DoLabel("Animated extent");
        snprintf(label, sizeof(label), "%.2g", target_extent_);
        _im_gui->DoSlider(label, 0.f, kTargetRange, &target_extent_);

        snprintf(label, sizeof(label), "x %.2g", target_offset_.x);
        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
                          &target_offset_.x);
        snprintf(label, sizeof(label), "y %.2g", target_offset_.y);
        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
                          &target_offset_.y);
        snprintf(label, sizeof(label), "z %.2g", target_offset_.z);
        _im_gui->DoSlider(label, -kTargetRange, kTargetRange,
                          &target_offset_.z);
      }
    }

    {  // Offset position
      static bool opened = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Aim offset", &opened);
      if (opened) {
        const float kOffsetRange = 2.5f;
        snprintf(label, sizeof(label), "x %.2g", aim_offset_.x);
        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.x);
        snprintf(label, sizeof(label), "y %.2g", aim_offset_.y);
        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.y);
        snprintf(label, sizeof(label), "z %.2g", aim_offset_.z);
        _im_gui->DoSlider(label, -kOffsetRange, kOffsetRange, &aim_offset_.z);
      }
    }

    // Options
    {
      _im_gui->DoCheckBox("Show skin", &show_skin_);
      _im_gui->DoCheckBox("Show joints", &show_joints_);
      _im_gui->DoCheckBox("Show target", &show_target_);
      _im_gui->DoCheckBox("Show aim offset", &show_aim_offset_);
      _im_gui->DoCheckBox("Show forward", &show_forward_);
      _im_gui->DoCheckBox("Show calibrated basis", &show_calibrated_basis_);
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    const ozz::math::Float3 radius(target_extent_ * .8f);
    _bound->min = target_offset_ - radius;
    _bound->max = target_offset_ + radius;
  }
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Look at with calibrated aim basis";
  return LookAtSampleApplication().Run(_argc, _argv, "1.0", title);
}*/
















//
// Simple bone pitch rotate distribution
//
#include <cstring>
#include <cmath>

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/mesh.h"
#include "framework/renderer.h"
#include "framework/utils.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"

#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(skeleton,
                           "Path to the skeleton (ozz archive format).",
                           "media/ct1.ozz", false)

// Animation archive can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(animation,
                           "Path to the animation (ozz archive format).",
                           "media/pistol_idle.ozz", false)

// The 3 spine joints we want to rotate for pitch.
const char* kJointNames[] = {"mixamorig:Spine", "mixamorig:Spine1",
                             "mixamorig:Spine2"};

const int kChainCount = 3;

// In the calibration pose, define "pitch axis" as model-space +X.
// This means "looking up/down" rotates around the character's right axis.
const ozz::math::SimdFloat4 kModelPitchAxis = ozz::math::simd_float4::x_axis();

class SpinePitchSampleApplication : public ozz::sample::Application 
{
 private:

  ozz::sample::PlaybackController controller_;
  ozz::animation::Skeleton skeleton_;
  ozz::animation::Animation animation_;
  ozz::animation::SamplingJob::Context context_;
  ozz::vector<ozz::math::SoaTransform> locals_;
  ozz::vector<ozz::math::Float4x4> models_;

  // Spine joint indices.
  int spine_joints_[kChainCount] = {-1, -1, -1};

  // Calibrated local pitch axis for each spine joint.
  ozz::math::SimdFloat4 local_pitch_axes_[kChainCount];

  // User controls.
  bool enable_pitch_ = true;
  float pitch_degrees_ = 0.f;
  float pitch_weights_[kChainCount] = {0.20f, 0.35f, 0.45f};

  // Display options.
  bool show_joints_ = false;
  bool show_axes_ = false;
  bool show_calibrated_axes_ = true;

 private:

static ozz::math::SimdQuaternion ExtractQuaternionLane(const ozz::math::SoaQuaternion& q, int lane) 
{
    float x[4];
    float y[4];
    float z[4];
    float w[4];

    ozz::math::StorePtrU(q.x, x);
    ozz::math::StorePtrU(q.y, y);
    ozz::math::StorePtrU(q.z, z);
    ozz::math::StorePtrU(q.w, w);

    ozz::math::SimdQuaternion out;
    out.xyzw = ozz::math::simd_float4::Load(x[lane], y[lane], z[lane], w[lane]);

    return out;
  }

  static void MultiplyJointLocalRotation(int joint, const ozz::math::SimdQuaternion& delta, ozz::span<ozz::math::SoaTransform> transforms) 
  {
    const int soa_index = joint / 4;
    const int lane = joint % 4;

    ozz::math::SoaTransform& t = transforms[soa_index];
    ozz::math::SimdQuaternion current = ExtractQuaternionLane(t.rotation, lane);
    const ozz::math::SimdQuaternion result = delta * current;

    t.rotation.x = ozz::math::SetI(t.rotation.x, ozz::math::SplatX(result.xyzw), lane);
    t.rotation.y = ozz::math::SetI(t.rotation.y, ozz::math::SplatY(result.xyzw), lane);
    t.rotation.z = ozz::math::SetI(t.rotation.z, ozz::math::SplatZ(result.xyzw), lane);
    t.rotation.w = ozz::math::SetI(t.rotation.w, ozz::math::SplatW(result.xyzw), lane);
  }

  int FindJoint(const char* name) const 
  {
    for (int i = 0; i < skeleton_.num_joints(); ++i) 
    {
      if (std::strcmp(skeleton_.joint_names()[i], name) == 0)
        return i;
    }

    return -1;
  }

  bool CalibratePitchAxesFromReferencePose() 
  {
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    ozz::animation::SamplingJob::Context calib_context;
    calib_context.Resize(num_joints);

    ozz::vector<ozz::math::SoaTransform> calib_locals;
    calib_locals.resize(num_soa_joints);

    ozz::vector<ozz::math::Float4x4> calib_models;
    calib_models.resize(num_joints);

    // Sample first frame / reference pose.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.context = &calib_context;
    sampling_job.ratio = 0.f;
    sampling_job.output = make_span(calib_locals);

    if (!sampling_job.Run()) 
    {
      ozz::log::Err() << "Failed to sample calibration pose." << std::endl;
      return false;
    }

    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(calib_locals);
    ltm_job.output = make_span(calib_models);

    if (!ltm_job.Run()) 
    {
      ozz::log::Err() << "Failed to build calibration model transforms." << std::endl;
      return false;
    }

    // Convert the desired model-space pitch axis into each joint's local space.
    for (int i = 0; i < kChainCount; ++i) 
    {
      const int joint = spine_joints_[i];
      const ozz::math::Float4x4 inv_joint = Invert(calib_models[joint]);
      local_pitch_axes_[i] = ozz::math::Normalize3(TransformVector(inv_joint, kModelPitchAxis));
    }

    return true;
  }

  void ApplySpinePitch(float pitch_radians) 
  {
    for (int i = 0; i < kChainCount; ++i) 
    {
      const int joint = spine_joints_[i];
      const float joint_pitch = pitch_radians * pitch_weights_[i];

      const ozz::math::SimdQuaternion delta = ozz::math::SimdQuaternion::FromAxisAngle(local_pitch_axes_[i], ozz::math::simd_float4::Load1(joint_pitch));

      MultiplyJointLocalRotation(joint, delta, make_span(locals_));
    }
  }

 protected:
  bool OnInitialize() override 
  {
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_))
      return false;

    if (!ozz::sample::LoadAnimation(OPTIONS_animation, &animation_))
      return false;

    for (int i = 0; i < kChainCount; ++i) 
    {
      spine_joints_[i] = FindJoint(kJointNames[i]);

      if (spine_joints_[i] < 0) 
      {
        ozz::log::Err() << "Joint not found: " << kJointNames[i] << std::endl;
        return false;
      }
    }

    locals_.resize(skeleton_.num_soa_joints());
    models_.resize(skeleton_.num_joints());
    context_.Resize(skeleton_.num_joints());

    if (!CalibratePitchAxesFromReferencePose())
      return false;

    return true;
  }

  bool OnUpdate(float _dt, float /*_time*/) override 
  {
    controller_.Update(animation_, _dt);

    // Sample base animation.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = &animation_;
    sampling_job.context = &context_;
    sampling_job.ratio = controller_.time_ratio();
    sampling_job.output = make_span(locals_);
    if (!sampling_job.Run()) 
      return false;

    // Apply local-space pitch to spine bones.
    if (enable_pitch_) 
    {
      const float pitch_radians = pitch_degrees_ * 3.14159265358979323846f / 180.f;
      ApplySpinePitch(pitch_radians);
    }

    // Build model-space matrices from modified locals.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);
    if (!ltm_job.Run())
      return false;

    return true;
  }

  bool OnDisplay(ozz::sample::Renderer* _renderer) override 
  {
    bool success = true;

    const ozz::math::Float4x4 identity = ozz::math::Float4x4::identity();
    success &= _renderer->DrawPosture(skeleton_, make_span(models_), identity);

    const float kAxeScale = 0.10f;
    const ozz::math::Float4x4 kAxesScale = ozz::math::Float4x4::Scaling(ozz::math::simd_float4::Load1(kAxeScale));

    if (show_joints_ || show_axes_) 
    {
      for (int i = 0; i < kChainCount; ++i) 
      {
        const ozz::math::Float4x4& joint_tm = models_[spine_joints_[i]];

        if (show_axes_) 
          success &= _renderer->DrawAxes(joint_tm * kAxesScale);

        if (show_joints_) 
          success &= _renderer->DrawSphereIm(0.02f, joint_tm, ozz::sample::kWhite);
      }
    }

    if (show_calibrated_axes_) 
    {
      for (int i = 0; i < kChainCount; ++i) 
      {
        const ozz::math::Float4x4& joint_tm = models_[spine_joints_[i]];

        ozz::math::Float3 axis;
        ozz::math::Store3PtrU(local_pitch_axes_[i], &axis.x);

        ozz::math::Float3 line[2] = {{0.f, 0.f, 0.f}, axis * 0.30f};

        success &= _renderer->DrawLines(line, ozz::sample::kGreen, joint_tm);
      }
    }

    return success;
  }

  bool OnGui(ozz::sample::ImGui* _im_gui) override 
  {
    char label[64];

    _im_gui->DoCheckBox("Enable pitch", &enable_pitch_);

    snprintf(label, sizeof(label), "Pitch degrees %.2f", pitch_degrees_);
    _im_gui->DoSlider(label, -80.f, 80.f, &pitch_degrees_);

    snprintf(label, sizeof(label), "Spine weight %.2f", pitch_weights_[0]);
    _im_gui->DoSlider(label, 0.f, 1.f, &pitch_weights_[0]);

    snprintf(label, sizeof(label), "Spine1 weight %.2f", pitch_weights_[1]);
    _im_gui->DoSlider(label, 0.f, 1.f, &pitch_weights_[1]);

    snprintf(label, sizeof(label), "Spine2 weight %.2f", pitch_weights_[2]);
    _im_gui->DoSlider(label, 0.f, 1.f, &pitch_weights_[2]);

    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);

      if (open)
        controller_.OnGui(animation_, _im_gui);
    }

    _im_gui->DoCheckBox("Show joints", &show_joints_);
    _im_gui->DoCheckBox("Show axes", &show_axes_);
    _im_gui->DoCheckBox("Show calibrated axes", &show_calibrated_axes_);

    return true;
  }

  void GetSceneBounds(ozz::math::Box* _bound) const override 
  {
    _bound->min = ozz::math::Float3(-2.f, -2.f, -2.f);
    _bound->max = ozz::math::Float3(2.f, 2.f, 2.f);
  }
};

int main(int _argc, const char** _argv) 
{
  const char* title = "Ozz-animation sample: Spine pitch by calibrated local axes";
  return SpinePitchSampleApplication().Run(_argc, _argv, "1.0", title);
}