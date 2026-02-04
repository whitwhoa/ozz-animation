//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "framework/application.h"
#include "framework/imgui.h"
#include "framework/renderer.h"
#include "framework/utils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"

// Skeleton archive can be specified as an option.
// OZZ_OPTIONS_DECLARE_STRING(skeleton,
//                           "Path to the skeleton (ozz archive format).",
//                           "media/skeleton.ozz", false)

// Animation archive can be specified as an option.
// OZZ_OPTIONS_DECLARE_STRING(animation,
//                           "Path to the animation (ozz archive format).",
//                           "media/animation.ozz", false)

class PlaybackSampleApplication : public ozz::sample::Application 
{
 private:
  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Runtime animation.
  ozz::animation::Animation animation_;

  // Sampling context.
  ozz::animation::SamplingJob::Context context_;

  // Buffer of local transforms as sampled from animation_.
  ozz::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  ozz::vector<ozz::math::Float4x4> models_;

 protected:
  virtual bool OnInitialize() 
  {
    // Reading skeleton.
    const char* skeletonPath = "media/skeleton.ozz";
    if (!ozz::sample::LoadSkeleton(skeletonPath, &skeleton_)) 
        return false;

    // Reading animation.
    const char* animationPath = "media/animation.ozz";
    if (!ozz::sample::LoadAnimation(animationPath, &animation_)) 
        return false;

    // Skeleton and animation needs to match.
    if (skeleton_.num_joints() != animation_.num_tracks())
        return false;

    // Allocates runtime buffers.
    const int num_soa_joints = skeleton_.num_soa_joints();
    locals_.resize(num_soa_joints);
    const int num_joints = skeleton_.num_joints();
    models_.resize(num_joints);

    // Allocates a context that matches animation requirements.
    context_.Resize(num_joints);

    return true;
  }

  // Updates current animation time and skeleton pose.
  virtual bool OnUpdate(float _dt, float) 
  {
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

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;
    ltm_job.input = make_span(locals_);
    ltm_job.output = make_span(models_);

    if (!ltm_job.Run()) 
        return false;

    return true;
  }

  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) 
  {
    return _renderer->DrawPosture(skeleton_, make_span(models_), ozz::math::Float4x4::identity());
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) 
  {
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose oc(_im_gui, "Animation control", &open);

      if (open) controller_.OnGui(animation_, _im_gui);
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const 
  {
    ozz::sample::ComputePostureBounds(make_span(models_), ozz::math::Float4x4::identity(), _bound);
  }
};

int main(int _argc, const char** _argv) {
  const char* title =  "Ozz-animation sample: Binary animation/skeleton playback";
  return PlaybackSampleApplication().Run(_argc, _argv, "1.0", title);
}





/////////////////////////////////////////////////////////////////////////////////////
// Interpolation example
/////////////////////////////////////////////////////////////////////////////////////

//// Full example: Ozz animation instance with three update modes:
////
//// 1) SimInterpolated: fixed sim ticks sample pose snapshots (prev/curr), render
//// interpolates. 2) SimStep:         fixed sim ticks sample pose (curr only),
//// render uses last sampled pose. 3) RenderOnly:      render dt advances time
//// and samples pose for visuals only (no sim influence).
////
//// Notes:
//// - Ozz does NOT require locals to contain "last tick" semantics. It's just
//// output storage.
//// - SamplingJob::Context is the stateful cache. Do not sample concurrently on
//// the same context.
//// - This example is single-threaded like the Ozz samples.
////
//// This is intended to be a drop-in style adaptation of the Ozz playback sample
//// you posted.
//
//#include "framework/application.h"
//#include "framework/imgui.h"
//#include "framework/renderer.h"
//#include "framework/utils.h"
//#include "ozz/animation/runtime/animation.h"
//#include "ozz/animation/runtime/local_to_model_job.h"
//#include "ozz/animation/runtime/sampling_job.h"
//#include "ozz/animation/runtime/skeleton.h"
//#include "ozz/base/maths/simd_math.h"
//#include "ozz/base/maths/soa_transform.h"
//#include "ozz/base/maths/vec_float.h"
//
//enum class AnimUpdateMode { SimInterpolated, SimStep, RenderOnly };
//
//// -------- SoA pose interpolation helpers (lerp T/S, nlerp R) ----------------
//
//static OZZ_INLINE ozz::math::SoaFloat3 LerpSoaFloat3(const ozz::math::SoaFloat3& a, const ozz::math::SoaFloat3& b, const ozz::math::SimdFloat4& t) 
//{
//  ozz::math::SoaFloat3 out;
//  out.x = a.x + (b.x - a.x) * t;
//  out.y = a.y + (b.y - a.y) * t;
//  out.z = a.z + (b.z - a.z) * t;
//
//  return out;
//}
//
//static OZZ_INLINE ozz::math::SoaQuaternion NLerpSoaQuaternion(const ozz::math::SoaQuaternion& a, ozz::math::SoaQuaternion b, const ozz::math::SimdFloat4& t) 
//{
//  // Dot product per lane
//  const ozz::math::SimdFloat4 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
//
//  // If dot < 0, negate b to take shortest path
//  const ozz::math::SimdInt4 flip = ozz::math::CmpLt(dot, ozz::math::simd_float4::zero());
//
//  b.x = ozz::math::Select(flip, -b.x, b.x);
//  b.y = ozz::math::Select(flip, -b.y, b.y);
//  b.z = ozz::math::Select(flip, -b.z, b.z);
//  b.w = ozz::math::Select(flip, -b.w, b.w);
//
//  // Nlerp
//  ozz::math::SoaQuaternion q;
//  q.x = a.x + (b.x - a.x) * t;
//  q.y = a.y + (b.y - a.y) * t;
//  q.z = a.z + (b.z - a.z) * t;
//  q.w = a.w + (b.w - a.w) * t;
//
//  // Normalize
//  const ozz::math::SimdFloat4 len2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
//
//  const ozz::math::SimdFloat4 inv_len = ozz::math::simd_float4::one() / ozz::math::Sqrt(len2);
//
//  q.x = q.x * inv_len;
//  q.y = q.y * inv_len;
//  q.z = q.z * inv_len;
//  q.w = q.w * inv_len;
//
//  return q;
//}
//
//static void InterpolateLocals(const ozz::vector<ozz::math::SoaTransform>& prev, 
//    const ozz::vector<ozz::math::SoaTransform>& curr, float alpha, ozz::vector<ozz::math::SoaTransform>* out) 
//{
//  const int n = static_cast<int>(prev.size());
//  out->resize(n);
//
//  const ozz::math::SimdFloat4 t = ozz::math::simd_float4::Load1(alpha);
//
//  for (int i = 0; i < n; ++i) 
//  {
//    const ozz::math::SoaTransform& A = prev[i];
//    const ozz::math::SoaTransform& B = curr[i];
//    ozz::math::SoaTransform& O = (*out)[i];
//
//    O.translation = LerpSoaFloat3(A.translation, B.translation, t);
//    O.scale = LerpSoaFloat3(A.scale, B.scale, t);
//    O.rotation = NLerpSoaQuaternion(A.rotation, B.rotation, t);
//  }
//}
//
//// ---------------------------- OzzAnimInstance --------------------------------
//
//struct OzzAnimInstance 
//{
//  AnimUpdateMode mode = AnimUpdateMode::SimInterpolated;
//
//  const ozz::animation::Skeleton* skeleton = nullptr;
//  const ozz::animation::Animation* animation = nullptr;
//
//  // Drives time ratio (0..1). In your engine you might replace this with your
//  // own time source.
//  ozz::sample::PlaybackController controller;
//
//  // Sampling context cache (stateful).
//  ozz::animation::SamplingJob::Context ctx;
//
//  // Two storage buffers for sim snapshots (double buffer).
//  ozz::vector<ozz::math::SoaTransform> locals_a;
//  ozz::vector<ozz::math::SoaTransform> locals_b;
//
//  // Pointers defining roles:
//  // - sim_prev: previous fixed tick pose snapshot
//  // - sim_curr: current  fixed tick pose snapshot (after sampling)
//  ozz::vector<ozz::math::SoaTransform>* sim_prev = nullptr;
//  ozz::vector<ozz::math::SoaTransform>* sim_curr = nullptr;
//
//  // Render scratch pose and models (what we draw).
//  ozz::vector<ozz::math::SoaTransform> render_locals;
//  ozz::vector<ozz::math::Float4x4> render_models;
//
//  bool Init(const ozz::animation::Skeleton* skel, const ozz::animation::Animation* anim) 
//  {
//    skeleton = skel;
//    animation = anim;
//
//    if (!skeleton || !animation) 
//        return false;
//
//    if (skeleton->num_joints() != animation->num_tracks()) 
//        return false;
//
//    ctx.Resize(skeleton->num_joints());
//
//    const int num_soa = skeleton->num_soa_joints();
//    locals_a.resize(num_soa);
//    locals_b.resize(num_soa);
//    render_locals.resize(num_soa);
//
//    render_models.resize(skeleton->num_joints());
//
//    sim_prev = &locals_a;
//    sim_curr = &locals_b;
//
//    // Initialize both snapshots to the same pose (time 0).
//    controller.Update(*animation, 0.0f);
//
//    if (!SampleInto(*sim_prev)) 
//        return false;
//
//    if (!SampleInto(*sim_curr)) 
//        return false;
//
//    // Also initialize render_locals so RenderOnly can start clean.
//    render_locals = *sim_curr;
//
//    return true;
//  }
//
//  bool SampleInto(ozz::vector<ozz::math::SoaTransform>& dst) 
//  {
//    ozz::animation::SamplingJob job;
//    job.animation = animation;
//    job.context = &ctx;
//    job.ratio = controller.time_ratio();
//    job.output = make_span(dst);
//
//    return job.Run();
//  }
//
//  bool LocalToModel(const ozz::vector<ozz::math::SoaTransform>& locals) 
//  {
//    ozz::animation::LocalToModelJob ltm;
//    ltm.skeleton = skeleton;
//    ltm.input = make_span(locals);
//    ltm.output = make_span(render_models);
//
//    return ltm.Run();
//  }
//
//  // Called by the fixed logic loop. Only meaningful for sim-driven modes.
//  bool SimTick(float fixed_dt) 
//  {
//    if (mode == AnimUpdateMode::RenderOnly) 
//    {
//      // RenderOnly does not participate in sim. It can remain frozen or be
//      // driven by render.
//      return true;
//    }
//
//    if (mode == AnimUpdateMode::SimInterpolated) 
//    {
//      // Promote old current snapshot to previous snapshot.
//      std::swap(sim_prev, sim_curr);
//    }
//    // SimStep: no need to keep prev, but sampling into sim_curr is still fine.
//
//    controller.Update(*animation, fixed_dt);
//
//    // Write new current snapshot into sim_curr.
//    return SampleInto(*sim_curr);
//  }
//
//  // Called by render. Builds render_models based on the current mode.
//  bool RenderBuild(float render_dt, float alpha_for_interp) 
//  {
//    if (mode == AnimUpdateMode::RenderOnly) 
//    {
//      // Drive time on render dt and sample directly for visuals.
//      controller.Update(*animation, render_dt);
//
//      if (!SampleInto(render_locals)) 
//          return false;
//
//      return LocalToModel(render_locals);
//    }
//
//    if (mode == AnimUpdateMode::SimStep) 
//    {
//      // No interpolation: just display the latest sim snapshot.
//      return LocalToModel(*sim_curr);
//    }
//
//    // SimInterpolated
//    InterpolateLocals(*sim_prev, *sim_curr, alpha_for_interp, &render_locals);
//
//    return LocalToModel(render_locals);
//  }
//};
//
//// --------------------------- Application
//// --------------------------------------
//
//class PlaybackModesApplication : public ozz::sample::Application 
//{
// private:
//  // Runtime skeleton/animation loaded from archives.
//  ozz::animation::Skeleton skeleton_;
//  ozz::animation::Animation animation_;
//
//  // One instance for demonstration. Your engine would have many instances.
//  OzzAnimInstance inst_;
//
//  // Fixed timestep.
//  float fixed_dt_ = 1.0f / 60.0f;
//  float accumulator_ = 0.0f;
//
// protected:
//  bool OnInitialize() override 
//  {
//    const char* skeletonPath = "media/skeleton.ozz";
//    if (!ozz::sample::LoadSkeleton(skeletonPath, &skeleton_)) 
//        return false;
//
//    const char* animationPath = "media/animation.ozz";
//    if (!ozz::sample::LoadAnimation(animationPath, &animation_)) 
//        return false;
//
//    if (!inst_.Init(&skeleton_, &animation_)) 
//        return false;
//
//    // Choose a default mode.
//    inst_.mode = AnimUpdateMode::SimInterpolated;
//
//    return true;
//  }
//
//  bool OnUpdate(float dt, float) override 
//  {
//    // Fixed-step simulation
//    accumulator_ += dt;
//
//    // Optional hitch clamp.
//    const float max_accum = fixed_dt_ * 8.0f;
//    if (accumulator_ > max_accum) 
//        accumulator_ = max_accum;
//
//    while (accumulator_ >= fixed_dt_) 
//    {
//      if (!inst_.SimTick(fixed_dt_)) 
//          return false;
//
//      accumulator_ -= fixed_dt_;
//    }
//
//    // Alpha for interpolated render state.
//    float alpha = (fixed_dt_ > 0.0f) ? (accumulator_ / fixed_dt_) : 0.0f;
//
//    if (alpha < 0.0f) 
//        alpha = 0.0f;
//
//    if (alpha > 1.0f) 
//        alpha = 1.0f;
//
//    // Build render posture depending on mode.
//    if (!inst_.RenderBuild(dt, alpha)) 
//        return false;
//
//    return true;
//  }
//
//  bool OnDisplay(ozz::sample::Renderer* renderer) override 
//  {
//    return renderer->DrawPosture(skeleton_, make_span(inst_.render_models), ozz::math::Float4x4::identity());
//  }
//
//bool OnGui(ozz::sample::ImGui* im_gui) override 
//{
//    static bool open = true;
//    ozz::sample::ImGui::OpenClose oc(im_gui, "Animation control", &open);
//    if (open) 
//    {
//      // Ozz sample playback GUI
//      inst_.controller.OnGui(animation_, im_gui);
//
//      // Mode selector
//      im_gui->DoLabel("Update mode:");
//
//      int mode_i = static_cast<int>(inst_.mode);
//
//      im_gui->DoRadioButton(0, "SimInterpolated (fixed + interpolated render)", &mode_i, true);
//      im_gui->DoRadioButton(1, "SimStep (fixed, no interpolation)", &mode_i, true);
//      im_gui->DoRadioButton(2, "RenderOnly (render-driven)", &mode_i, true);
//
//      inst_.mode = static_cast<AnimUpdateMode>(mode_i);
//
//      // Fixed dt slider
//      im_gui->DoLabel("Fixed dt:");
//      // pow: use 1.0f for linear slider response
//      im_gui->DoSlider("fixed_dt", 1.0f / 240.0f, 1.0f / 15.0f, &fixed_dt_, 1.0f, true);
//    }
//
//    return true;
//  }
//
//
//
//  void GetSceneBounds(ozz::math::Box* bound) const override 
//  {
//    ozz::sample::ComputePostureBounds(make_span(inst_.render_models), ozz::math::Float4x4::identity(), bound);
//  }
//};
//
//int main(int argc, const char** argv) 
//{
//  const char* title = "Ozz example: fixed sim + render modes (SimInterpolated/SimStep/RenderOnly)";
//  return PlaybackModesApplication().Run(argc, argv, "1.0", title);
//}