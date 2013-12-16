#pragma once

class FontMap;
class VectorShape;
class Res;
class AtlasImageMap;

const FontMap* GetFont(int id);
FontMap* _GetFont(int id);
const VectorShape* GetShape(int id);
Res* GetSound(int id);
const AtlasImageMap* GetTexture(int id);
