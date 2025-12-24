/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Scott Moreau <oreaus@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <wayfire/core.hpp>  // ADD THIS - needed for wf::get_core()
#include <wayfire/view.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/plugins/animate/animate.hpp>
#include <wayfire/plugins/common/shared-core-data.hpp>
#include "dali.hpp"
#include "mercury.hpp"

class wayfire_phodius_extras : public wf::plugin_interface_t
{
    wf::shared_data::ref_ptr_t<wf::animate::animate_effects_registry_t> effects_registry;
    wf::option_wrapper_t<wf::animation_description_t> mercury_duration{"phodius-extras/mercury_duration"};
    wf::option_wrapper_t<wf::animation_description_t> dali_duration{"phodius-extras/dali_duration"};

  public:
    void init() override
    {
        LOGI("phodius-extras: init() called");
        
        if (!wf::get_core().is_gles2())
        {
            LOGE("phodius-extras: not supported on non-gles2 wayfire");
            return;
        }

        LOGI("phodius-extras: registering effects");

        effects_registry->register_effect("mercury", wf::animate::effect_description_t{
            .generator = [] { return std::make_unique<wf::mercury::mercury_animation>(); },
            .default_duration = [this] { return mercury_duration.value(); },  // [this] not [=]
        });

        effects_registry->register_effect("dali", wf::animate::effect_description_t{
            .generator = [] { return std::make_unique<wf::dali::dali_animation>(); },
            .default_duration = [this] { return dali_duration.value(); },  // [this] not [=]
        });

        LOGI("phodius-extras: effects registered successfully");
    }

    void fini() override
    {
        effects_registry->unregister_effect("mercury");
        effects_registry->unregister_effect("dali");
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_phodius_extras);