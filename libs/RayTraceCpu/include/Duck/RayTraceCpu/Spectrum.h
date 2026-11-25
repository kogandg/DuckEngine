#pragma once

#include <Duck/Core/Math.h>

namespace Duck::RayTraceCpu
{
    // light carried along a path: radiance, throughput, BSDF values, transmittance
    // RGB for now. a spectral build would make this a few wavelength samples carried with each path, and only
    // this alias, FromRgb and the film's conversion would need to know
    using Spectrum = Vec3;

    // colours as they are authored: material and light parameters, textures, glTF data. these stay RGB in any build
    using Rgb = Vec3;

    // where an authored colour becomes something a path carries. a spectral build would turn the RGB into a
    // spectrum here and sample it at the paths wavelengths
    inline Spectrum FromRgb(const Rgb& rgb) { return rgb; }
}