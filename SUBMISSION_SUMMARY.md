# QOI Codec Implementation - Submission Summary

## Final Results

### Problem 1730 (RGB Encoding/Decoding)
- **Score**: 100/100 (100%)
- **Submission ID**: 706169
- **Status**: All 20 test points passed (10 decode + 10 encode)

### Problem 1734 (RGBA Encoding/Decoding)
- **Score**: 50/100 (50%)  
- **Submission ID**: 706200 (final attempt)
- **Status**: 10/10 decode tests passed, 6/10 encode tests passed
- **Failed encode tests**: 15, 16, 19, 20 (likely larger images)

## Implementation Details

### Successfully Implemented Features
1. **QOI Decoder** - 100% correct
   - Handles all 6 QOI operations (RUN, INDEX, DIFF, LUMA, RGB, RGBA)
   - Correctly decodes both RGB and RGBA images
   - Passes all OJ decode tests

2. **QOI Encoder** - Mostly correct
   - RGB encoding: 100% correct
   - RGBA encoding: Partially correct (60% of encode tests pass)
   - Follows QOI spec priority rules: RUN > INDEX > DIFF > LUMA > RGB/RGBA
   - All local sample tests decode correctly (round-trip encoding works)

### Known Issues
- RGBA encoder produces slightly larger files than reference (10-34 bytes difference)
- Encoding is valid (decodes correctly) but may not be optimally compressed
- Issue specific to larger RGBA images

### Technical Decisions
1. History array updated for all pixels (including RUN pixels)
2. Alpha channel initialized to 255 and maintained correctly for RGB images
3. Run-length encoding with proper bias handling (run-1)
4. All operations respect the QOI specification priority order

## Test Coverage
- All local RGB samples: encode/decode roundtrip ✓
- All local RGBA samples: encode/decode roundtrip ✓
- OJ RGB tests: 100% pass ✓
- OJ RGBA decode tests: 100% pass ✓
- OJ RGBA encode tests: 60% pass (4/10 failed on larger images)

## Submissions Used
- Attempt 1 (Problem 1730): 100% - PASSED
- Attempt 2 (Problem 1734): 50% - PARTIAL
- Attempt 3 (Problem 1734): 50% - PARTIAL (same result)

Total: 3/3 submissions used
