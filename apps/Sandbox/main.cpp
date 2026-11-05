#include <Duck/Core/Assert.h>
#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/Log.h>

using namespace Duck;

int main()
{
    Framebuffer fb(256, 256);
    for (std::uint32_t y = 0; y < fb.Height(); ++y)
    {
        for (std::uint32_t x = 0; x < fb.Width(); ++x)
        {
            fb[x, y] = PackRgba8(Vec3(x / 255.0f, y / 255.0f, 0.25f));
        }
    }

    Log::Info("Bottom-right pixel: {:#010x}", fb[255, 255]);
    DUCK_ASSERT(fb.Width() == 256);

    // DUCK_VERIFY(fb.Height() == 100, "Height was {}", fb.Height());
}