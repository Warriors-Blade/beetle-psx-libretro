
#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <gl>
#include <vector>
#include <stdint.h>

#include "../retrogl/retrogl.h"
#include "../retrogl/error.h"
#include "../retrogl/buffer.h"
#include "../retrogl/shader.h"
#include "../retrogl/program.h"
#include "../retrogl/types.h"
#include "../retrogl/texture.h"
#include "../retrogl/framebuffer.h"

#include "../libretro.h"

extern unsigned int VRAM_WIDTH_PIXELS
extern unsigned int VRAM_HEIGHT

/// How many vertices we buffer before forcing a draw
unsigned int VERTEX_BUFFER_LEN = 2048;

// Helper structs are used because C/C++ syntax doesn't allow
// statements like e.g. (u32, u32) frontend_resolution = (640, 480); 
struct FrontendResolution {
    uint32_t w;
    uint32_t h;
};

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class GlRenderer
{
public:
    /// Buffer used to handle PlayStation GPU draw commands
    DrawBuffer<CommandVertex*>* command_buffer;
    /// Primitive type for the vertices in the command buffers
    /// (TRIANGLES or LINES)
    GLenum command_draw_mode;
    /// Temporary buffer holding vertices for semi-transparent draw
    /// commands.
    std::vector<CommandVertex*> semi_transparent_vertices;
    /// Transparency mode for semi-transparent commands
    SemiTransparencyMode semi_transparency_mode;
    /// Polygon mode (for wireframe)
    GLenum command_polygon_mode;
    /// Buffer used to draw to the frontend's framebuffer
    DrawBuffer<OutputVertex*>* output_buffer;
    /// Buffer used to copy textures from `fb_texture` to `fb_out`
    DrawBuffer<ImageLoadVertex*>* image_load_buffer;
    /// Texture used to store the VRAM for texture mapping
    DrawConfig* config;
    /// Framebuffer used as a shader input for texturing draw commands
    Texture* fb_texture;
    /// Framebuffer used as an output when running draw commands
    Texture* fb_out;
    /// Depth buffer for fb_out
    Texture* fb_out_depth;
    /// Current resolution of the frontend's framebuffer
    FrontendResolution frontend_resolution;
    /// Current internal resolution upscaling factor
    uint32_t internal_upscaling;
    /// Current internal color depth
    uint8_t internal_color_depth;
    /// Counter for preserving primitive draw order in the z-buffer
    /// since we draw semi-transparent primitives out-of-order.
    int16_t primitive_ordering;

    GlRenderer();

    //// pub fn from_config(config: DrawConfig) -> Result<GlRenderer, Error>
    GlRenderer(DrawConfig config);

    ~GlRenderer();

    static template<typename T>
    DrawBuffer<T>* build_buffer<T>( const char* vertex_shader,
                                    const char* fragment_shader,
                                    size_t capacity,
                                    bool lifo  );

    void draw();
    void apply_scissor();
    void bind_libretro_framebuffer();
    void upload_textures( TopLeft topleft, Dimensions dimensions,
                          uint16_t* pixel_buffer[VRAM_PIXELS]);

    void upload_vram_window( TopLeft top_left, Dimensions dimensions,
                             uint16_t* pixel_buffer[VRAM_PIXELS]);

    DrawConfig* draw_config();
    void prepare_render();
    bool refresh_variables();
    void finalize_frame();
    void maybe_force_draw(  size_t nvertices, GLenum draw_mode, 
                            bool semi_transparent, 
                            SemiTransparencyMode semi_transparency_mode);

    void set_draw_offset(int16_t x, int16_t y);

    void set_draw_area(TopLeft top_left, Dimensions dimensions);
    void set_display_mode(TopLeft top_left, 
                          Resolution resolution, bool depth_24bpp);

    void push_triangle( CommandVertex v[3],
                        SemiTransparencyMode semi_transparency_mode);

    void push_line( CommandVertex v[2],
                    SemiTransparencyMode semi_transparency_mode);

    void fill_rect(Color color, TopLeft top_left, Dimensions dimensions);
    void copy_rect( TopLeft source_top_left, 
                    TopLeft target_top_left, Dimensions dimensions);

};


//// TODO: These have to be classes with ctors so it plays nice with
//// push_slice() and initializer lists
class CommandVertex {
    /// Position in PlayStation VRAM coordinates
    int16_t position[3];
    /// RGB color, 8bits per component
    uint8_t color[3];
    /// Texture coordinates within the page
    uint16_t texture_coord[2];
    /// Texture page (base offset in VRAM used for texture lookup)
    uint16_t texture_page[2];
    /// Color Look-Up Table (palette) coordinates in VRAM
    uint16_t clut[2];
    /// Blending mode: 0: no texture, 1: raw-texture, 2: texture-blended
    uint8_t texture_blend_mode;
    /// Right shift from 16bits: 0 for 16bpp textures, 1 for 8bpp, 2
    /// for 4bpp
    uint8_t depth_shift;
    /// True if dithering is enabled for this primitive
    uint8_t dither;
    /// 0: primitive is opaque, 1: primitive is semi-transparent
    uint8_t semi_transparent;

    //from_vertex
    CommandVertex(Vertex& v);
};

class OutputVertex {
    /// Vertex position on the screen
    float position[2];
    /// Corresponding coordinate in the framebuffer
    uint16_t fb_coord[2];
};

class ImageLoadVertex {
    // Vertex position in VRAM
    uint16_t position[2];
};

enum class SemiTransparencyMode {
    /// Source / 2 + destination / 2
    Average = 0;
    /// Source + destination
    Add = 1;
    /// Destination - source
    SubstractSource = 2;
    /// Destination + source / 4
    AddQuarterSource = 3;
};

#endif