/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Scott Moreau <oreaus@gmail.com>
 * Copyright (c) 2025 Andrew Pliatsikas <futurebytestore@gmail.com>
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

#include <wayfire/core.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/output.hpp>
#include <wayfire/render-manager.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/util/duration.hpp>
#include <wayfire/plugins/animate/animate.hpp>

static const char *mercury_vert_source =
    R"(
#version 100

attribute highp vec2 position;
attribute highp vec2 uv_in;

varying highp vec2 uvpos;

void main() {

    gl_Position = vec4(position.xy, 0.0, 1.0);
    uvpos = uv_in;
}
)";


static const char *mercury_frag_source =
    R"(
#version 100
@builtin_ext@
@builtin@
precision highp float;
varying highp vec2 uvpos;
uniform vec2 size;
uniform float progress;
uniform float mercury_speed;
uniform float mercury_width;
uniform float mercury_height;
uniform vec4 mercury_color;

// ============================================
// RIPPLE DISSOLUTION EFFECT
// ============================================

#define PI 3.14159265359

// Wave function: solution to 2D wave equation
float wave(vec2 uv, vec2 center, float time, float frequency, float speed) {
    float r = length(uv - center);
    if (r < 0.001) r = 0.001; 
    
    float k = 20.0 * PI * frequency;
    float omega = k * speed;
    float amplitude = 1.0 / sqrt(r + 0.1);
    
    return amplitude * cos(k * r - omega * time);
}

// Superposition of multiple wave sources
float wave_field(vec2 uv, float time, float freq_base) {
    float field = 0.0;
    
    vec2 sources[10];
    sources[0] = vec2(0.5, 0.5);
    sources[1] = vec2(0.2, 0.3);
    sources[2] = vec2(0.8, 0.3);
    sources[3] = vec2(0.3, 0.8);
    sources[4] = vec2(0.7, 0.7);
    sources[5] = vec2(0.1, 0.1);
    sources[6] = vec2(0.2, 0.7);
    sources[7] = vec2(0.9, 0.3);
    sources[8] = vec2(0.0, 0.5);
    sources[9] = vec2(0.7, 0.2);
    
    for (int i = 0; i < 10; i++) {
        float phase_offset = float(i) * 0.7;
        float freq = freq_base * (1.0 + float(i) * 0.1);
        field += wave(uv, sources[i], time + phase_offset, freq, mercury_speed * 0.15);
    }
    
    return field / 10.0; 
}

// Gradient magnitude for edge glow
float gradient_magnitude(vec2 uv, float time, float freq) {
    float eps = 0.0005;
    float dudx = (wave_field(uv + vec2(eps, 0.0), time, freq) - 
                  wave_field(uv - vec2(eps, 0.0), time, freq)) / (2.0 * eps);
    float dudy = (wave_field(uv + vec2(0.0, eps), time, freq) - 
                  wave_field(uv - vec2(0.0, eps), time, freq)) / (2.0 * eps);
    return sqrt(dudx * dudx + dudy * dudy);
}

float smooth_threshold(float x, float edge, float width) {
    return 1.0 / (1.0 + exp(-(x - edge) / width));
}

// Soft edge vignette - fades to transparent near window boundaries
float edge_fade(vec2 uv, float softness) {
    float fade_left = smoothstep(0.0, softness, uv.x);
    float fade_right = smoothstep(0.0, softness, 1.0 - uv.x);
    float fade_bottom = smoothstep(0.0, softness, uv.y);
    float fade_top = smoothstep(0.0, softness, 1.0 - uv.y);
    
    return fade_left * fade_right * fade_bottom * fade_top;
}

void main()
{
    // Hard clip outside bounds
    if (uvpos.x < 0.0 || uvpos.x > 1.0 || uvpos.y < 0.0 || uvpos.y > 1.0) {
        gl_FragColor = vec4(0.0);
        return;
    }
    
    // Calculate soft edge fade
    float soft_edge = 1.0;
    
    if (soft_edge < 0.001) {
        gl_FragColor = vec4(0.0);
        return;
    }
    
    // Sample original texture first
    vec4 original = get_pixel(uvpos);
    
    float effect_progress;
    float t;
    
    effect_progress = progress;
    t = progress * mercury_speed * 0.1;
    
    
    vec2 uv = uvpos;
    uv.x *= size.x / size.y;
    
    float frequency = 1.0 + mercury_width * 0.05 * (1.0 - progress);
    
    // Compute wave field
    float field = wave_field(uv, t, frequency);
    float grad = gradient_magnitude(uv, t, frequency);
    
    // DYNAMIC THRESHOLD based on effect_progress
    float threshold_min = -1.0;
    float threshold_max = 4.0;
    float dissolve_threshold = mix(threshold_min, threshold_max * (1.0 - progress), effect_progress);
    
    // Calculate mask
    float softness = 0.1 * max(0.01, mercury_height);
    float dissolve = smooth_threshold(field, dissolve_threshold, softness);
    
    float effect_blend = smoothstep(0.0, 0.0, effect_progress);
    
    // Fade out glow as effect completes
    float glow_fadeout = 1.0 - smoothstep(0.85, 0.99, effect_progress);
    
    // Distortion ramps up then back down
    // Peak distortion at progress 0.5, zero at 0 and 1
    float distortion_strength = sin(progress * 3.14159);
    
    // Edge calculation for glow
    float edge_proximity = 2.0 - abs(field - dissolve_threshold) * 50.0;
    edge_proximity = clamp(edge_proximity, 0.0, 1.0);
    
    float edge_glow = edge_proximity * grad * 2.0 * effect_blend * glow_fadeout;
    
    // Distortion - peaks in middle, zero at start and end
    float distort_amount = field * 0.2 * distortion_strength;
    vec2 final_uv = uvpos + vec2(distort_amount);
    final_uv = clamp(final_uv, 0.0, 1.0);
    
    vec4 tex = get_pixel(final_uv);
    
    // Colors
    vec3 ripple_color = mercury_color.rgb;
    vec3 glow_color = mix(ripple_color, vec3(1.0), 0.5); 
    
    // COMPOSITION
    vec4 result;
    
    float final_mask = mix(1.0, dissolve, effect_blend);
    
    result.rgb = mix(tex.rgb, glow_color * glow_fadeout, edge_glow * 0.6);
    
    float peak_glow = max(0.0, field - dissolve_threshold - 0.1) * 0.0001 * effect_blend * glow_fadeout;
    result.rgb += ripple_color * peak_glow * glow_fadeout;

    result.a = tex.a * final_mask;
    result.a = max(result.a, edge_glow * final_mask * glow_fadeout);
    
    gl_FragColor = clamp(result, 0.0, 1.0);
}
)";

   
namespace wf
{
namespace mercury
{
using namespace wf::scene;
using namespace wf::animate;
using namespace wf::animation;

static std::string mercury_transformer_name = "mercury";

wf::option_wrapper_t<double> mercury_mercury_speed{"phodius-extras/mercury_speed"};
wf::option_wrapper_t<double> mercury_mercury_width{"phodius-extras/mercury_width"};
wf::option_wrapper_t<double> mercury_mercury_height{"phodius-extras/mercury_height"};
wf::option_wrapper_t<wf::color_t> mercury_mercury_color{"phodius-extras/mercury_color"};

class mercury_transformer : public wf::scene::view_2d_transformer_t
{
  public:
    wayfire_view view;
    wf::output_t *output;
    OpenGL::program_t program;
    wf::auxilliary_buffer_t buffer;
    duration_t progression;

    class simple_node_render_instance_t : public wf::scene::transformer_render_instance_t<transformer_base_node_t>
    {
        wf::signal::connection_t<node_damage_signal> on_node_damaged =
            [=] (node_damage_signal *ev)
        {
            push_to_parent(ev->region);
        };

        mercury_transformer *self;
        wayfire_view view;
        damage_callback push_to_parent;

      public:
        simple_node_render_instance_t(mercury_transformer *self, damage_callback push_damage,
            wayfire_view view) : wf::scene::transformer_render_instance_t<transformer_base_node_t>(self,
                push_damage,
                view->get_output())
        {
            this->self = self;
            this->view = view;
            this->push_to_parent = push_damage;
            self->connect(&on_node_damaged);
        }

        ~simple_node_render_instance_t()
        {}

        void schedule_instructions(
            std::vector<render_instruction_t>& instructions,
            const wf::render_target_t& target, wf::region_t& damage) override
        {
            instructions.push_back(render_instruction_t{
                        .instance = this,
                        .target   = target,
                        .damage   = damage & self->get_bounding_box(),
                    });
        }

        void transform_damage_region(wf::region_t& damage) override
        {
            damage |= self->get_bounding_box();
        }

        void render(const wf::scene::render_instruction_t& data) override
        {
            auto bb  = self->get_children_bounding_box();
            auto pbb = self->get_padded_bounding_box();
            auto tex = wf::gles_texture_t{get_texture(1.0)};

            const float vertices[] = {
                -1.0, -1.0,
                1.0f, -1.0,
                1.0f, 1.0f,
                -1.0, 1.0f
            };
            wf::pointf_t offset1{-float(bb.x - pbb.x) / bb.width,
                -float(pbb.height - ((bb.y - pbb.y) + bb.height)) / bb.height};
            wf::pointf_t offset2{float(pbb.width) / bb.width + offset1.x,
                float(pbb.height) / bb.height + offset1.y};
            const float uv[] = {
                float(offset1.x), float(offset2.y),
                float(offset2.x), float(offset2.y),
                float(offset2.x), float(offset1.y),
                float(offset1.x), float(offset1.y),
            };
            auto progress = self->progression.progress();

            data.pass->custom_gles_subpass([&]
            {
                self->buffer.allocate({pbb.width, pbb.height});
                wf::gles::bind_render_buffer(self->buffer.get_renderbuffer());
                wf::gles_texture_t final_tex{self->buffer.get_texture()};
                OpenGL::clear(wf::color_t{0.0, 0.0, 0.0, 0.0}, GL_COLOR_BUFFER_BIT);
                self->program.use(wf::TEXTURE_TYPE_RGBA);
                self->program.attrib_pointer("position", 2, 0, vertices);
                self->program.attrib_pointer("uv_in", 2, 0, uv);
                self->program.uniform2f("size", bb.width * 1.0, bb.height * 1.0);
                self->program.uniform1f("progress", 1.0 - progress);
                self->program.uniform1f("mercury_speed", mercury_mercury_speed);
                self->program.uniform1f("mercury_width", mercury_mercury_width);
                self->program.uniform1f("mercury_height", mercury_mercury_height);
             

                glm::vec4 mercury_color{
                    wf::color_t(mercury_mercury_color).r,
                    wf::color_t(mercury_mercury_color).g,
                    wf::color_t(mercury_mercury_color).b,
                    wf::color_t(mercury_mercury_color).a};
                self->program.uniform4f("mercury_color", mercury_color);

                self->program.set_active_texture(tex);
                GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

                wf::gles::bind_render_buffer(data.target);
                for (auto box : data.damage)
                {
                    wf::gles::render_target_logic_scissor(data.target, wlr_box_from_pixman_box(box));
                    OpenGL::render_transformed_texture(final_tex, pbb,
                        wf::gles::render_target_orthographic_projection(data.target),
                        glm::vec4(1.0, 1.0, 1.0, std::clamp((progress - 0.07) * 10.0, 0.0, 1.0)), 0);
                }

                GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
                self->program.deactivate();
                self->buffer.free();
            });
        }
    };

    mercury_transformer(wayfire_view view, wf::geometry_t bbox,
        wf::animation_description_t duration) : wf::scene::view_2d_transformer_t(view)
    {
        this->view = view;
        this->progression = duration_t{wf::create_option(duration)};
        if (view->get_output())
        {
            output = view->get_output();
            output->render->add_effect(&pre_hook, wf::OUTPUT_EFFECT_PRE);
        }

        wf::gles::run_in_context([&]
        {
            program.compile(mercury_vert_source, mercury_frag_source);
        });
    }

    wf::geometry_t get_padded_bounding_box()
    {
        auto box     = this->get_children_bounding_box();
        auto padding = 100;
        box.x     -= padding;
        box.y     -= padding;
        box.width += padding * 2;
        box.height += padding * 2;
        return box;
    }

    wf::geometry_t get_bounding_box() override
    {
        return get_padded_bounding_box();
    }

    wf::effect_hook_t pre_hook = [=] ()
    {
        output->render->damage(this->get_bounding_box());
    };

    void gen_render_instances(std::vector<render_instance_uptr>& instances,
        damage_callback push_damage, wf::output_t *shown_on) override
    {
        instances.push_back(std::make_unique<simple_node_render_instance_t>(
            this, push_damage, view));
    }

    void init_animation(bool mercury)
    {
        if (mercury)
        {
            this->progression.reverse();
        }

        this->progression.start();
    }

    virtual ~mercury_transformer()
    {
        if (output)
        {
            output->render->rem_effect(&pre_hook);
        }

        wf::gles::run_in_context_if_gles([&]
        {
            program.free_resources();
        });
    }
};

class mercury_animation : public animation_base_t
{
    wayfire_view view;

  public:
    void init(wayfire_view view, wf::animation_description_t dur, animation_type type) override
    {
        this->view = view;
        pop_transformer(view);
        auto bbox = view->get_transformed_node()->get_bounding_box();
        auto tmgr = view->get_transformed_node();
        auto node = std::make_shared<mercury_transformer>(view, bbox, dur);
        tmgr->add_transformer(node, wf::TRANSFORMER_HIGHLEVEL + 1, mercury_transformer_name);
        node->init_animation(type & WF_ANIMATE_HIDING_ANIMATION);
    }

    void pop_transformer(wayfire_view view)
    {
        if (view->get_transformed_node()->get_transformer(mercury_transformer_name))
        {
            view->get_transformed_node()->rem_transformer(mercury_transformer_name);
        }
    }

    bool step() override
    {
        if (!view)
        {
            return false;
        }

        auto tmgr = view->get_transformed_node();
        if (!tmgr)
        {
            return false;
        }

        if (auto tr =
                tmgr->get_transformer<wf::mercury::mercury_transformer>(mercury_transformer_name))
        {
            auto running = tr->progression.running();
            if (!running)
            {
                pop_transformer(view);
                return false;
            }

            return running;
        }

        return false;
    }

    void reverse() override
    {
        if (auto tr =
                view->get_transformed_node()->get_transformer<wf::mercury::mercury_transformer>(
                    mercury_transformer_name))
        {
            tr->progression.reverse();
        }
    }
};
}
}
