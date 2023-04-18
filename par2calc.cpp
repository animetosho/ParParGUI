#include "par2calc.h"
#include "settings.h"

static int simpleCountFromSize(quint64 size, const SrcFileList& files)
{
    int count = 0;
    for(const auto& file : files) {
        count += (file.size() + size-1) / size;
    }
    return count;
}

quint64 Par2Calc::sliceSizeFromCount(int& count, quint64 multiple, int limit, const SrcFileList& files, int fileCount, int moveDir)
{
    if(files.isEmpty()) { // should never happen
        count = 0;
        return 0;
    }
    if(fileCount > limit) { // impossible to satisfy
        return 0;
    }
    if(count > limit)
        count = limit;

    // from par2gen.js/calcSliceSizeForFiles:
    // there may be a better algorithm to do this, but we'll use a binary search approach to find the correct number of slices to use
    quint64 lbound = multiple;
    quint64 ubound = 0;
    for(const auto& file : files)
        if(file.size() > ubound)
            ubound = file.size();

    if(ubound == 0) // all files are empty
        return 0;

    if(count < fileCount) {
        count = fileCount;
        return ubound;
    }

    auto mod = ubound % multiple;
    if(mod)
        ubound += multiple - mod;

    while(lbound < ubound - multiple) {
        quint64 mid = ((ubound + lbound) / (multiple*2)) * multiple;
        int testCount = simpleCountFromSize(mid, files);
        if(count >= testCount)
            ubound = mid;
        else
            lbound = mid;
    }

    int lboundSlices = simpleCountFromSize(lbound, files);
    int uboundSlices = simpleCountFromSize(ubound, files);
    if(lboundSlices == count)
        return lbound;
    if(uboundSlices == count)
        return ubound;

    if(lboundSlices > limit) {
        // higher slice count is invalid, must use lower count
        count = uboundSlices;
        return ubound;
    }

    // prefer closer target (as long as we're not specifically moving in a direction, i.e. SpinBox)
    if(moveDir == 0) {
        int lboundDiff = abs(lboundSlices-count);
        int uboundDiff = abs(uboundSlices-count);
        if(lboundDiff < uboundDiff) {
            count = lboundSlices;
            return lbound;
        } else if(uboundDiff < lboundDiff) {
            count = uboundSlices;
            return ubound;
        }
    }

    if(moveDir > 0) {
        count = lboundSlices;
        return lbound;
    } else {
        // we generally prefer the lower slice count, since we know we can't exceed limits that way
        count = uboundSlices;
        return ubound;
    }
}

int Par2Calc::sliceCountFromSize(quint64& size, quint64 multiple, int limit, const SrcFileList& files, int fileCount)
{
    int mod = size % multiple;
    if(mod)
        size += multiple - mod;

    if(size == 0)
        size = 4;

    int count = simpleCountFromSize(size, files);
    if(count > limit) {
        size = sliceSizeFromCount(count, multiple, limit, files, fileCount);
    }
    return count;
}

int Par2Calc::maxSliceCount(quint64 multiple, int limit, const SrcFileList& files)
{
    int count = 0;
    for(const auto& file : files) {
        count += (file.size() + multiple-1) / multiple;
        if(count > limit) return limit;
    }
    return count;
}


int Par2Calc::round_down_pow2(int v) {
    if((v & (v-1)) == 0) return v; // is a power of 2 (shortcut exists because this is the common case)

    // find target number via a float conversion
    // (usage of float over double does mean that this can be wrong for very large numbers, but we're not expecting these)
    union { float f; uint32_t u; } tmp;
    tmp.f = (float)v;     // convert to float
    tmp.u &= 0xff800000;  // discard mantissa
    return (int)tmp.f; // convert back to int
}

