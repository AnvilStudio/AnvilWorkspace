#pragma once
#include <Render/RenderData.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <limits>
#include <vector>
namespace anv {
inline void AppendUIQuads(std::vector<QuadSubmission>& world,
 const std::vector<UIRectSubmission>& ui, const glm::mat4& vp, uint32_t width, uint32_t height) {
 if (!width || !height || ui.empty()) return;
 const auto inv = glm::inverse(vp);
 auto convert = [&](glm::vec2 p) {
   glm::vec4 ndc(2.f*p.x/width-1.f, 1.f-2.f*p.y/height, 0.f, 1.f);
   glm::vec4 v = inv*ndc; return glm::vec2(v)/v.w;
 };
 for (auto& rect: ui) {
   if (rect.Size.x<=0 || rect.Size.y<=0) continue;
   auto tl=convert(rect.Position), tr=convert(rect.Position+glm::vec2(rect.Size.x,0.f)),
        bl=convert(rect.Position+glm::vec2(0.f,rect.Size.y));
   auto x=tr-tl, y=bl-tl;
   QuadSubmission quad{};
   quad.Position=tl+(x+y)*0.5f;
   quad.Rotation=glm::degrees(std::atan2(x.y,x.x));
   quad.Size={glm::length(x),glm::length(y)};
   quad.Color=rect.Color;
   quad.TextureAsset=rect.TextureAsset;
   quad.UVMin=rect.UVMin;
   quad.UVMax=rect.UVMax;
   quad.Layer=std::numeric_limits<int>::max();
   world.push_back(quad);
 }
}
}
