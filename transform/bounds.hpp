#pragma once

#include <vector>

#include "transform.hpp"
#include "../maniac/symbol.hpp"

class ColorRangesBounds : public ColorRanges
{
protected:
    std::vector<std::pair<ColorVal, ColorVal> > bounds;
    const ColorRanges *ranges;
public:
    ColorRangesBounds(const std::vector<std::pair<ColorVal, ColorVal> > &boundsIn, const ColorRanges *rangesIn) : bounds(boundsIn), ranges(rangesIn) {}
    bool isStatic() const { return false; }
    int numPlanes() const { return bounds.size(); }
    ColorVal min(int p) const { assert(p<numPlanes()); return std::max(ranges->min(p), bounds[p].first); }
    ColorVal max(int p) const { assert(p<numPlanes()); return std::min(ranges->max(p), bounds[p].second); }
    void minmax(const int p, const prevPlanes &pp, ColorVal &min, ColorVal &max) const {
        assert(p<numPlanes());
        if (p==0) { min=bounds[p].first; max=bounds[p].second; return; } // optimization for special case
        ranges->minmax(p, pp, min, max);
        if (min < bounds[p].first) min=bounds[p].first;
        if (max > bounds[p].second) max=bounds[p].second;
        if (min>max) {
           // should happen only if alpha=0 interpolation produces YI combination for which Q range from ColorRangesYIQ is outside bounds
           min=bounds[p].first;
           max=bounds[p].second;
        }
        assert(min <= max);
    }
};


template <typename IO>
class TransformBounds : public Transform<IO> {
protected:
    std::vector<std::pair<ColorVal, ColorVal> > bounds;

    bool undo_redo_during_decode() { return false; }

    const ColorRanges *meta(Images&, const ColorRanges *srcRanges) {
        if (srcRanges->isStatic()) {
            return new StaticColorRanges(bounds);
        } else {
            return new ColorRangesBounds(bounds, srcRanges);
        }
    }

    bool load(const ColorRanges *srcRanges, RacIn<IO> &rac) {
        SimpleSymbolCoder<SimpleBitChance, RacIn<IO>, 24> coder(rac);
        bounds.clear();
        for (int p=0; p<srcRanges->numPlanes(); p++) {
//            ColorVal min = coder.read_int(0, srcRanges->max(p) - srcRanges->min(p)) + srcRanges->min(p);
//            ColorVal max = coder.read_int(0, srcRanges->max(p) - min) + min;
            ColorVal min = coder.read_int(srcRanges->min(p), srcRanges->max(p));
            ColorVal max = coder.read_int(min, srcRanges->max(p));
            if (min > max) return false;
            if (min < srcRanges->min(p)) return false;
            if (max > srcRanges->max(p)) return false;
            bounds.push_back(std::make_pair(min,max));
            v_printf(5,"[%i:%i..%i]",p,min,max);
        }
        return true;
    }

#ifdef HAS_ENCODER
    void save(const ColorRanges *srcRanges, RacOut<IO> &rac) const {
        SimpleSymbolCoder<SimpleBitChance, RacOut<IO>, 24> coder(rac);
        for (int p=0; p<srcRanges->numPlanes(); p++) {
            ColorVal min = bounds[p].first;
            ColorVal max = bounds[p].second;
//            coder.write_int(0, srcRanges->max(p) - srcRanges->min(p), min - srcRanges->min(p));
//            coder.write_int(0, srcRanges->max(p) - min, max - min);
            coder.write_int(srcRanges->min(p), srcRanges->max(p), min);
            coder.write_int(min, srcRanges->max(p), max);
            v_printf(5,"[%i:%i..%i]",p,min,max);
        }
    }

    bool process(const ColorRanges *srcRanges, const Images &images) {
        bounds.clear();
        bool trivialbounds=true;
        int nump=srcRanges->numPlanes();
        for (int p=0; p<nump; p++) {
            ColorVal min = srcRanges->max(p);
            ColorVal max = srcRanges->min(p);
            for (const Image& image : images)
            for (uint32_t r=0; r<image.rows(); r++) {
                for (uint32_t c=0; c<image.cols(); c++) {
                    if (nump>3 && p<3 && image(3,r,c)==0) continue; // don't take fully transparent pixels into account
                    ColorVal v = image(p,r,c);
                    if (v < min) min = v;
                    if (v > max) max = v;
                    assert(v <= srcRanges->max(p));
                    assert(v >= srcRanges->min(p));
                }
            }
            if (min > max) min = max = (min+max)/2; // this can happen if the image is fully transparent
            bounds.push_back(std::make_pair(min,max));
            if (min > srcRanges->min(p)) trivialbounds=false;
            if (max < srcRanges->max(p)) trivialbounds=false;
        }
        return !trivialbounds;
    }
#endif
};
