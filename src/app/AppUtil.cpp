/*
 Copyright 2019-2020 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "AppCore.h"

// Enable if we want simple CPU metrics in GUI
//#define GUI_CPU_STATS

// TODO: Maybe move resolution tables to another file? To be included directly?

struct UpscalingTechniqueInfo {
  UpscalingTechniqueKey id;
  std::string name;
};

struct ResolutionInfo {
    ResolutionInfoKey id;
    VkExtent2D resolution_extent;
    std::string text;
};

struct TargetResolutionChain {
    TargetResolutionKey id;
    ResolutionInfoKey resolution_info_key;
    std::vector<ResolutionInfoKey> internal_resolution_info_keys;
};

struct PresentResolutionChain {
    PresentResolutionKey id;
    ResolutionInfoKey resolution_info_key;
    std::vector<TargetResolutionKey> target_resolution_keys;
};

static UpscalingTechniqueInfo
    s_upscaling_techniques[UpscalingTechniqueKey::kuCount] = {
        {UpscalingTechniqueKey::None, "None"},
        {UpscalingTechniqueKey::CAS, "FidelityFX CAS"},
};

static ResolutionInfo s_resolution_infos[ResolutionInfoKey::krCount] = {
    {
        ResolutionInfoKey::kr540p, {960, 540}, "960 x 540"
    },
    {
        ResolutionInfoKey::kr720p, {1280, 720}, "1280 x 720"
    },
    {
        ResolutionInfoKey::kr1080p, {1920, 1080}, "1920 x 1080"
    },
    {
        ResolutionInfoKey::kr1440p, {2560, 1440}, "2560 x 1440"
    },
    {
        ResolutionInfoKey::kr2160p, {3840, 2160}, "3840 x 2160"
    },
};

static TargetResolutionChain s_target_resolutions[TargetResolutionKey::ktCount] = {
    {
        TargetResolutionKey::kt1080p, ResolutionInfoKey::kr1080p, {ResolutionInfoKey::kr1080p, ResolutionInfoKey::kr720p, ResolutionInfoKey::kr540p}
    },
    {
        TargetResolutionKey::kt2160p, ResolutionInfoKey::kr2160p, {ResolutionInfoKey::kr2160p, ResolutionInfoKey::kr1440p, ResolutionInfoKey::kr1080p}
    },
};

static PresentResolutionChain s_present_resolutions[PresentResolutionKey::kpCount] = {
    {
        PresentResolutionKey::kp1080p, ResolutionInfoKey::kr1080p, {TargetResolutionKey::kt1080p},
    },
    {
        PresentResolutionKey::kp2160p, ResolutionInfoKey::kr2160p, {TargetResolutionKey::kt2160p, TargetResolutionKey::kt1080p},
    },
};

PresentResolutionKey VkexInfoApp::FindPresentResolutionKey(const uint32_t width)
{
    PresentResolutionKey detected_present_key = PresentResolutionKey::kpCount;

    for (auto& present_chain : s_present_resolutions) {
        auto res_info_key = present_chain.resolution_info_key;
        if (width == s_resolution_infos[res_info_key].resolution_extent.width) {
            detected_present_key = present_chain.id;
            break;
        }
    }

    VKEX_ASSERT(detected_present_key != PresentResolutionKey::kpCount);

    return detected_present_key;
}

void VkexInfoApp::SetPresentResolution(PresentResolutionKey new_present_resolution)
{
    m_present_resolution_key = new_present_resolution;

    m_selected_target_resolution_index = 0;
    m_target_resolution_key = s_present_resolutions[m_present_resolution_key].target_resolution_keys[m_selected_target_resolution_index];

    m_selected_internal_resolution_index = 0;
    m_internal_resolution_key = s_target_resolutions[m_target_resolution_key].internal_resolution_info_keys[m_selected_internal_resolution_index];
}

void VkexInfoApp::UpdateTargetResolutionState()
{
    auto old_key = m_target_resolution_key;

    m_target_resolution_key = s_present_resolutions[m_present_resolution_key].target_resolution_keys[m_selected_target_resolution_index];

    if (old_key != m_target_resolution_key) {
        m_selected_internal_resolution_index = 0;
        UpdateInternalResolutionState();
    }
}

void VkexInfoApp::UpdateInternalResolutionState()
{
    m_internal_resolution_key = s_target_resolutions[m_target_resolution_key].internal_resolution_info_keys[m_selected_internal_resolution_index];
}

void VkexInfoApp::UpdateUpscalingTechniqueState() {
  m_upscaling_technique_key =
      s_upscaling_techniques[m_selected_upscaling_technique_index].id;
}

UpscalingTechniqueKey VkexInfoApp::GetUpscalingTechnique() {
  return m_upscaling_technique_key;
}

const char *VkexInfoApp::GetUpscalingTechniqueText() {
  return s_upscaling_techniques[m_upscaling_technique_key].name.c_str();
}

VkExtent2D VkexInfoApp::GetInternalResolutionExtent()
{
    return s_resolution_infos[m_internal_resolution_key].resolution_extent;
}

VkExtent2D VkexInfoApp::GetTargetResolutionExtent()
{
    auto res_info_key = s_target_resolutions[m_target_resolution_key].resolution_info_key;
    return s_resolution_infos[res_info_key].resolution_extent;
}

VkExtent2D VkexInfoApp::GetPresentResolutionExtent()
{
    auto res_info_key = s_present_resolutions[m_present_resolution_key].resolution_info_key;
    return s_resolution_infos[res_info_key].resolution_extent;
}

const char * VkexInfoApp::GetTargetResolutionText()
{
    auto res_info_key = s_target_resolutions[m_target_resolution_key].resolution_info_key;
    return s_resolution_infos[res_info_key].text.c_str();
}

const char * VkexInfoApp::GetPresentResolutionText()
{
    auto res_info_key = s_present_resolutions[m_present_resolution_key].resolution_info_key;
    return s_resolution_infos[res_info_key].text.c_str();
}

void VkexInfoApp::BuildUpscalingTechniqueList(
    std::vector<const char *> &upscaling_technique_list) {
  for (const auto &technique : s_upscaling_techniques) {
    upscaling_technique_list.push_back(technique.name.c_str());
  }
}

void VkexInfoApp::BuildInternalResolutionTextList(std::vector<const char*>& internal_text_list)
{
    for (auto internal_resolution : s_target_resolutions[m_target_resolution_key].internal_resolution_info_keys) {
        auto& res_info = s_resolution_infos[internal_resolution];
        internal_text_list.push_back(res_info.text.c_str());
    }
}

void VkexInfoApp::BuildTargetResolutionTextList(std::vector<const char*>& target_text_list)
{
    for (auto target_resolution : s_present_resolutions[m_present_resolution_key].target_resolution_keys) {
        auto res_info_key = s_target_resolutions[target_resolution].resolution_info_key;
        auto& res_info = s_resolution_infos[res_info_key];
        target_text_list.push_back(res_info.text.c_str());
    }
}

void VkexInfoApp::IssueGpuTimeStart(vkex::CommandBuffer cmd, PerFrameData& per_frame_data, TimerTag tag)
{
    uint32_t slot = tag * 2;
    cmd->CmdWriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, per_frame_data.timer_query_pool, slot);
}

void VkexInfoApp::IssueGpuTimeEnd(vkex::CommandBuffer cmd, PerFrameData& per_frame_data, TimerTag tag)
{
    uint32_t slot = (tag * 2) + 1;
    cmd->CmdWriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, per_frame_data.timer_query_pool, slot);
}

void VkexInfoApp::ReadbackGpuTimestamps(uint32_t frame_index)
{
    auto& per_frame_data = m_per_frame_datas[frame_index];

    if (per_frame_data.timestamps_issued == false) {
        return;
    }

    per_frame_data.timestamps_issued = false;

    const uint32_t query_count = TimerTag::kTimerQueryCount;

    std::vector<uint64_t> data(query_count);
    const uint32_t stride = sizeof(uint64_t);
    const size_t   data_size = data.size() * stride;

    VkResult vk_result = vkex::GetQueryPoolResults(
        *GetDevice(),
        *per_frame_data.timer_query_pool,
        0,
        query_count,
        data_size,
        data.data(),
        stride,
        (VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    VKEX_ASSERT(vk_result == VK_SUCCESS);

    for (uint32_t tag_index = 0; tag_index < TimerTag::kTimerTagCount; tag_index++) {
        uint32_t slot_start_index = tag_index * 2;

        per_frame_data.issued_gpu_timers[tag_index].start_time  = data[slot_start_index + 0];
        per_frame_data.issued_gpu_timers[tag_index].end_time    = data[slot_start_index + 1];
    }
    
    per_frame_data.timestamps_readback = true;
}

double VkexInfoApp::CalculateGpuTimeRange(const PerFrameData& per_frame_data, TimerTag requested_range, double nano_scaler)
{
    const double timestamp_period = static_cast<double>(GetDevice()->GetPhysicalDevice()->GetPhysicalDeviceLimits().timestampPeriod);

    const auto& requested_timer_range = per_frame_data.issued_gpu_timers[requested_range];
    auto gpu_ticks = requested_timer_range.end_time - requested_timer_range.start_time;

    return (gpu_ticks * timestamp_period * nano_scaler);
}

vkex::uint3 VkexInfoApp::CalculateSimpleDispatchDimensions(GeneratedShaderState& gen_shader_state, VkExtent2D dest_image_extent)
{
    auto tg_dims = gen_shader_state.program->GetInterface().GetThreadgroupDimensions();

    vkex::uint3 dispatch_dims = { 0, 0, 1 };

    dispatch_dims.x = uint32_t(ceil((float(dest_image_extent.width) / tg_dims.x)));
    dispatch_dims.y = uint32_t(ceil((float(dest_image_extent.height) / tg_dims.y)));

    return dispatch_dims;
}

ImVec2 VkexInfoApp::GetSuggestedGUISize()
{
    ImVec2 gui_window_size(400, 400);

    auto present_extent = GetPresentResolutionExtent();
    if (present_extent.height == 2160) {
        gui_window_size.x *= 2;
        gui_window_size.y *= 2;
    }

    return gui_window_size;
}

float VkexInfoApp::GetSuggestedFontScale()
{
    float font_scale = 1.0f;

    auto present_extent = GetPresentResolutionExtent();
    if (present_extent.height == 2160) {
        font_scale = 2.f;
    }

    return font_scale;
}

void VkexInfoApp::DrawAppInfoGUI(uint32_t frame_index)
{
    if (!m_configuration.enable_imgui) {
        return;
    }

    ImVec2 gui_window_size = GetSuggestedGUISize();
    ImGui::SetNextWindowSize(gui_window_size, ImGuiCond_Once);

    if (ImGui::Begin("Application Info")) {

        auto font_scale = GetSuggestedFontScale();
        ImGui::SetWindowFontScale(font_scale);

        {
            ImGui::Columns(2);
            {
                ImGui::Text("Application PID");
                ImGui::NextColumn();
                ImGui::Text("%d", GetProcessId());
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Application Name");
                ImGui::NextColumn();
                ImGui::Text("%s", m_configuration.name.c_str());
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
        
        ImGui::Separator();

        {
            ImGui::Columns(2);
            {
                ImGui::Text("Animation");
                ImGui::NextColumn();
                ImGui::Checkbox("##AnimationEnabled", &m_animation_enabled);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }

        ImGui::Separator();

        // Upscale info
        {
            // TODO: Upscale selector
            // TODO: Visualizer selector
            //          Target res
            //          Upscaled
            //          Delta visualizers...
            ImGui::Columns(2);
            {
              std::vector<const char *> upscaling_techniques;
              BuildUpscalingTechniqueList(upscaling_techniques);

              ImGui::Text("Upscaling technique");
              ImGui::NextColumn();
              ImGui::Combo("##UpscalingTech",
                           (int *)(&m_selected_upscaling_technique_index),
                           upscaling_techniques.data(),
                           int(upscaling_techniques.size()));
              ImGui::NextColumn();
            }
            {
                std::vector<const char*> resolution_items;
                BuildInternalResolutionTextList(resolution_items);

                ImGui::Text("Internal resolution");
                ImGui::NextColumn();
                ImGui::Combo("##InternalRes", (int*)(&m_selected_internal_resolution_index), resolution_items.data(), int(resolution_items.size()));
                ImGui::NextColumn();
            }
            {
                std::vector<const char*> resolution_items;
                BuildTargetResolutionTextList(resolution_items);

                ImGui::Text("Target Resolution");
                ImGui::NextColumn();
                ImGui::Combo("##TargetRes", (int*)(&m_selected_target_resolution_index), resolution_items.data(), int(resolution_items.size()));
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Present Resolution");
                ImGui::NextColumn();
                ImGui::Text(GetPresentResolutionText());
                ImGui::NextColumn();
            }
            {
                std::vector<const char*> visualizer_items = {"Off", "Luma delta", "RGB delta"};
                VKEX_ASSERT(int(visualizer_items.size()) == int(DeltaVisualizerMode::kDeltaVizCount));
                ImGui::Text("Delta Visualizer");
                ImGui::NextColumn();
                ImGui::Combo("##DeltaViz", (int*)(&m_delta_visualizer_mode), visualizer_items.data(), int(visualizer_items.size()));
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }

        ImGui::Separator();

        auto& per_frame_data = m_per_frame_datas[frame_index];
        if (per_frame_data.timestamps_readback == true)
        {
            ImGui::Columns(2);
            {
                ImGui::Text("GPU Timers");
                ImGui::NextColumn();
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Internal");
                ImGui::NextColumn();
                ImGui::NextColumn();
            }
            {
                double ms_diff = CalculateGpuTimeRange(per_frame_data, TimerTag::kTotalInternal, VKEX_TIMER_NANOS_TO_MILLIS);

                ImGui::Text("  Total Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", ms_diff);
                ImGui::NextColumn();
            }
            {
                double ms_diff = CalculateGpuTimeRange(per_frame_data, TimerTag::kSceneRenderInternal, VKEX_TIMER_NANOS_TO_MILLIS);

                ImGui::Text("    Scene Draw Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", ms_diff);
                ImGui::NextColumn();
            }
            {
                double ms_diff = CalculateGpuTimeRange(per_frame_data, TimerTag::kUpscaleInternal, VKEX_TIMER_NANOS_TO_MILLIS);

                ImGui::Text("    Upscale Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", ms_diff);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Target");
                ImGui::NextColumn();
                ImGui::NextColumn();
            }
            {
                double ms_diff = CalculateGpuTimeRange(per_frame_data, TimerTag::kSceneRenderTarget, VKEX_TIMER_NANOS_TO_MILLIS);

                ImGui::Text("  Scene Render Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", ms_diff);
                ImGui::NextColumn();
            }
        }

        // TODO: Model picker?

#if defined(GUI_CPU_STATS)
        // TODO: Make these collapsable?
        ImGui::Separator();

        {
            ImGui::Columns(2);
            {
                ImGui::Text("CPU Stats");
                ImGui::NextColumn();
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Average Frame Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", (GetAverageFrameTime() * 1000.0));
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Current Frame Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", GetFrameElapsedTime() * 1000.0f);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Max Past %d Frames Time", kWindowFrames);
                ImGui::NextColumn();
                ImGui::Text("%f ms", GetMaxWindowFrameTime() * 1000.0f);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Min Past %d Frames Time", kWindowFrames);
                ImGui::NextColumn();
                ImGui::Text("%f ms", GetMinWindowFrameTime() * 1000.0f);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Frames Per Second");
                ImGui::NextColumn();
                ImGui::Text("%f fps", GetFramesPerSecond());
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Total Frames");
                ImGui::NextColumn();
                ImGui::Text("%llu frames", static_cast<unsigned long long>(GetElapsedFrames()));
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Elapsed Time (s)");
                ImGui::NextColumn();
                ImGui::Text("%f seconds", GetElapsedTime());
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }

        ImGui::Separator();

        // Function call times
        {
            ImGui::Columns(2);
            {
                ImGui::Text("Update Call Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", m_update_fn_time * 1000.0);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Render Call Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", m_render_fn_time * 1000.0f);
                ImGui::NextColumn();
            }
            {
                ImGui::Text("Present Call Time");
                ImGui::NextColumn();
                ImGui::Text("%f ms", m_present_fn_time * 1000.0f);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
        }
#endif // defined(GUI_CPU_STATS)
    }
    ImGui::End();
}
