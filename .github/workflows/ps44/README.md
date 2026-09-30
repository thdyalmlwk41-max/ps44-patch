# Days Gone 900p Patch - GoldHEN Plugin (SPRX)

Forces Days Gone (CUSA15027, EUR, v1.81) to run at 900p with reduced shadow/fog quality on PS4 9.00 + GoldHEN 2.4b18.10.

## Features
- **900p Resolution**: 75% screen percentage (1600x900 from 1080p)
- **Reduced Shadow Quality**: Medium (1) instead of Epic/High
- **Reduced View Distance**: Medium (1) - reduces fog/LOD distance
- **Reduced Post-Processing**: Medium (1) - reduces bloom, DOF, fog
- **Runtime Pattern Scanning**: Finds UE4 functions dynamically (ASLR-safe)
- **Auto Re-apply**: Re-applies settings if game resets them

## Requirements
- PS4 on 9.00 firmware
- GoldHEN 2.4b18.10+
- Days Gone CUSA15027 (EUR) v1.81

## Building

### Option 1: GitHub Actions (Easiest - No Local Setup)
1. Create new repo on GitHub
2. Push this folder (`ps44`) to the repo
3. GitHub Actions builds automatically
4. Download `days_gone_900p.sprx` from **Actions → Artifacts**

### Option 2: Docker (Local)
```bash
docker run --rm -v "C:/Users/ayoub/Desktop/ps44:/src" orbisdev/ps4-sdk:latest bash -c "
  cd /src && 
  cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/ps4/toolchain.cmake &&
  cmake --build build
"
```

## Installation
1. Copy `days_gone_900p.sprx` to PS4:
   ```
   /data/GoldHEN/plugins/days_gone_900p.sprx
   ```
2. Enable plugins in GoldHEN settings
3. Restart GoldHEN or reboot PS4
4. Launch Days Gone

## How It Works
1. **Finds game memory** via `sceKernelGetProcessVmMap` - locates main executable RX region
2. **Pattern scans** for UE4 strings (`SetResolutionScaleNormalized`, `SetShadowQuality`, etc.)
3. **Resolves functions** by finding RIP-relative references to strings, then walking back to function prologues
4. **Calls setters directly** on `UGameUserSettings` instance
5. **Hooks GameUserSettings tick** to re-apply settings periodically

## Target Settings
| Setting | Value | Description |
|---------|-------|-------------|
| Screen Percentage | 75% | 900p from 1080p |
| Shadow Quality | 1 (Medium) | 0=Low, 1=Med, 2=High, 3=Epic |
| View Distance | 1 (Medium) | Reduces fog/LOD |
| Post Process | 1 (Medium) | Reduces bloom, DOF, fog |

## Troubleshooting
**Plugin doesn't load:**
- Check `/data/GoldHEN/logs/` for `[days_gone_900p]` messages
- Verify SPRX in correct folder, plugins enabled in GoldHEN

**Settings don't apply:**
- Game may reset on resolution change - plugin re-applies every ~60 frames
- Check kernel log for "Settings applied" messages

**Crash on launch:**
- Pattern scan may have found wrong function
- Report crash dump + kernel log for analysis

## Technical Details
- **Entry**: `_start()` → finds game memory → resolves functions → applies settings
- **Pattern Scan**: Searches game RX region for string refs, finds RIP-relative code refs, walks back to `push rbp; mov rbp, rsp`
- **Hooking**: 14-byte absolute JMP detour with trampoline for original function
- **No stdlib**: Pure bare-metal, uses SCE kernel APIs only

## License
MIT - Use at your own risk. Modifies game memory at runtime.

## Credits
- OrbisDev team for PS4 SDK
- GoldHEN team for plugin loader
- UE4 reverse engineering community