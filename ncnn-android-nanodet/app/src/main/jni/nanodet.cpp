// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "nanodet.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cpu.h"

NanoDet::NanoDet()
{
    blob_pool_allocator.set_size_compare_ratio(0.f);
    workspace_pool_allocator.set_size_compare_ratio(0.f);
}

int NanoDet::load(AAssetManager* mgr, const char* modeltype, bool use_gpu)
{
    // 把原有的环境清空一下
    nanodet.clear();
    blob_pool_allocator.clear();
    workspace_pool_allocator.clear();

    ncnn::set_cpu_powersave(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());

    nanodet.opt = ncnn::Option();

#if NCNN_VULKAN
    nanodet.opt.use_vulkan_compute = use_gpu;
#endif

    nanodet.opt.num_threads = ncnn::get_big_cpu_count();
    nanodet.opt.blob_allocator = &blob_pool_allocator;
    nanodet.opt.workspace_allocator = &workspace_pool_allocator;

    // 加载模型和设置模型对应的一些参数
    char parampath[256];
    char modelpath[256];
    sprintf(parampath, "%s.param", modeltype);
    sprintf(modelpath, "%s.bin", modeltype);
    nanodet.load_param(mgr, parampath);
    nanodet.load_model(mgr, modelpath);

    return 0;
}

int NanoDet::detect(cv::Mat& rgb)
{
    ncnn::Mat in = ncnn::Mat::from_pixels_resize(rgb.data, ncnn::Mat::PIXEL_RGB, rgb.cols, rgb.rows,640, 480);
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = nanodet.create_extractor();
    ex.input("input", in);
    ncnn::Mat model_out;
    ex.extract("output", model_out);

    cv::Mat out(240, 320, CV_32FC1, model_out.data);
    double vmin, vmax, alpha;
    cv::minMaxLoc(out, &vmin, &vmax);
    alpha = 255.0 / (vmax - vmin);
    out.convertTo(out, CV_8UC1, alpha, -vmin * alpha);
    cv::Mat color;
    cv::applyColorMap(out, color, cv::COLORMAP_JET);

    cv::resize(color,rgb,rgb.size());

    return 0;
}

