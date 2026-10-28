static BOOL gMacRec = NO, gMacPlay = NO, gMacFiring = NO, gMacWait = NO;
static float gRunElapsed = 0, gMacPrev = 0;

static void macAdd(uint8_t type, int8_t arg) {
    if (!gMacRec || gMacFiring || gMacN >= MAC_MAX) return;
    gMac[gMacN].t = gRunElapsed;
    gMac[gMacN].type = type;
    gMac[gMacN].arg = arg;
    gMacN++;
}

HV(NearestLane, fl[F_NOPULL])

static bool (*o_Jump)(void *, void *, void *, void *);
static bool h_Jump(void *a, void *b, void *c, void *d) {
    if (fl[F_NOJUMP] && !gMacFiring) return false;
    macAdd(0, 0);
    return o_Jump(a, b, c, d);
}

static bool (*o_Dive)(void *, void *, void *, void *);
static bool h_Dive(void *a, void *b, void *c, void *d) {
    if (fl[F_NODIVE] && !gMacFiring) return false;
    macAdd(1, 0);
    return o_Dive(a, b, c, d);
}

static bool (*o_ChangeLane)(void *, int, void *, void *);
static bool h_ChangeLane(void *self, int off, void *c, void *d) {
    if (fl[F_NOLANE] && !gMacFiring) return false;
    macAdd(2, (int8_t)off);
    return o_ChangeLane(self, off, c, d);
}

static void (*o_Hover)(void *, void *, void *, void *);
static void h_Hover(void *a, void *b, void *c, void *d) {
    macAdd(3, 0);
    o_Hover(a, b, c, d);
}
HV(DizzyStars, fl[F_FX])
HV(HbCrashFx, fl[F_FX])
HV(RollDirt, fl[F_FX])
HV(BushLeaves, fl[F_FX])
HV(Grind, fl[F_FX])
HV(FlightPuff, fl[F_FX])
HV(PogoFx, fl[F_FX])
HV(BoosterFx, fl[F_FX])
HV(ReviveFx, fl[F_FX])
HV(ShowSneakers, fl[F_PROPS])
HV(ShowMagnet, fl[F_PROPS])
HV(ShowPogo, fl[F_PROPS])
HV(ShowFlight, fl[F_PROPS])
HV(ShowHoverboard, fl[F_PROPS])
HV(PlayStripes, fl[F_STRIPES])
HV(StopStripes, fl[F_STRIPES])
HV(CamImpact, fl[F_SHAKE])
HV(AudChangeTrack, fl[F_SND])
HV(AudStumble, fl[F_SND])
HV(AudCoin, fl[F_SND])
HV(AudHeadstart, fl[F_SND])
HV(AudBooster, fl[F_SND])
HV(TrainAudio, fl[F_SND])
HV(PickDetect, fl[F_NODET])
HV(AnimRoll, fl[F_ANIM])
HV(AnimDive, fl[F_ANIM])
HV(AnimLand, fl[F_ANIM])
HV(AnimFall, fl[F_ANIM])
HV(AnimDoubleJump, fl[F_ANIM])
HV(AnimJump, fl[F_ANIM])
HV(AnimLane, fl[F_ANIM])
HV(LaneRotation, fl[F_NOROT])
HB(CamFollow, fl[F_CAMFOLLOW], true)
HB(CamStop, fl[F_CAMSTOP], false)

static bool (*o_overEdge)(void *, float, float, void *, void *);
static bool h_overEdge(void *s, float a, float b, void *pos, void *mi) {
    if (fl[F_GOD5]) return true;
    return o_overEdge(s, a, b, pos, mi);
}

static float (*o_react)(void *, void *);
static float h_react(void *s, void *mi) {
    if (fl[F_ZEROREACT]) return 0.f;
    return o_react(s, mi);
}

static void (*o_setFov)(void *, float, void *);
static void h_setFov(void *cam, float v, void *mi) {
    if (fl[F_FOV] && gFov > 0) v = gFov;
    o_setFov(cam, v, mi);
}

static void (*o_setTS)(float, void *);
static void h_setTS(float v, void *mi) {
    if (fl[F_TS] && v != 0.f) v = gTSValue;
    o_setTS(v, mi);
}

static void (*o_setFps)(int, void *);
static void h_setFps(int v, void *mi) {
    if (fl[F_FPS] && gFpsLimit > 0) v = gFpsLimit;
    o_setFps(v, mi);
}

static BOOL gAudOn = NO, gAudSync = NO, gAudVolOn = NO;
static float gAudPitch = 0.05f, gAudVol = 1.f;

static float audioTarget(void) {
    float p = gAudPitch;
    if (gAudSync) {
        static float last = 1;
        void *b = callM(meth("UnityEngine", "Time", "get_timeScale", 0), NULL, NULL);
        float ts = b ? floatOf(b) : 1;
        if (ts > 0.001f) last = ts;
        p = last;
    }
    return MAX(0.01f, MIN(3.f, p));
}

static void audioSet(void *src) {
    static void *m;
    if (!m) m = meth("UnityEngine", "AudioSource", "set_pitch", 1);
    float p = audioTarget();
    void *a[1] = { &p };
    callOK(m, src, a);
}

static void (*o_aSetPitch)(void *, float, void *);
static void h_aSetPitch(void *s, float v, void *mi) {
    if (fl[F_AUDIO]) v = audioTarget();
    o_aSetPitch(s, v, mi);
}

static void (*o_aPlay)(void *, void *, void *, void *);
static void h_aPlay(void *s, void *b, void *c, void *d) {
    if (fl[F_AUDIO] && s) audioSet(s);
    o_aPlay(s, b, c, d);
}

static void (*o_aShot2)(void *, void *, float, void *);
static void h_aShot2(void *s, void *clip, float vol, void *mi) {
    if (fl[F_AUDIO] && s) audioSet(s);
    o_aShot2(s, clip, vol, mi);
}

static void (*o_aShot1)(void *, void *, void *, void *);
static void h_aShot1(void *s, void *clip, void *c, void *d) {
    if (fl[F_AUDIO] && s) audioSet(s);
    o_aShot1(s, clip, c, d);
}

static int (*o_getSeed)(void *, void *);
static int h_getSeed(void *s, void *mi) {
    int r = o_getSeed(s, mi);
    gLastSeed = r;
    return r;
}

static void (*o_CamGameplay)(void *, void *, void *, void *);
static void h_CamGameplay(void *, void *, void *, void *);
static void (*o_CamLate)(void *, void *, void *, void *);
static void h_CamLate(void *, void *, void *, void *);

typedef struct {
    unsigned long long mask;
    const char *ns, *cls, *m;
    int np;
    void *repl;
    void **orig;
    BOOL done;
} HK;

#define E(N, MASK, NS, CLS, M, NP) { MASK, NS, CLS, M, NP, (void *)h_##N, (void **)&o_##N, NO }

static HK gHK[] = {
    E(Stumble, B(F_NOSTUMBLE) | B(F_GOD5), "SYBO.Subway", "CharacterStumble", "Stumble", 1),
    E(Kill, B(F_GOD2), "SYBO.Subway", "CharacterStumble", "Kill", 1),
    E(IsAutoRevive, B(F_GOD3), "SYBO.Subway", "CharacterStumble", "IsAutoReviveEnabled", 0),
    E(overEdge, B(F_GOD5), "SYBO.RunnerCore.Character", "CharacterCollision", "CanGetOverEdge", 3),
    E(CheckFrontal, B(F_GOD1), "SYBO.RunnerCore.Character", "CharacterMotor", "CheckFrontalImpact", 1),
    E(CheckSide, B(F_GOD1), "SYBO.RunnerCore.Character", "CharacterMotor", "CheckSideImpact", 1),
    E(ApplyGravity, B(F_NOGRAV), "SYBO.RunnerCore.Character", "CharacterMotor", "ApplyGravity", 0),
    E(CancelLaneChanging, B(F_CLIPTRAIN) | B(F_NOPULL), "SYBO.RunnerCore.Character", "CharacterMotor", "CancelLaneChanging", 0),
    E(Jump, B(F_NOJUMP) | B(F_MACREC), "SYBO.RunnerCore.Character", "CharacterMotor", "Jump", 0),
    E(Dive, B(F_NODIVE) | B(F_MACREC), "SYBO.RunnerCore.Character", "CharacterMotor", "Dive", 0),
    E(ChangeLane, B(F_NOLANE) | B(F_MACREC), "SYBO.RunnerCore.Character", "CharacterMotor", "ChangeLane", 1),
    E(HandleRollingEnded, B(F_DIRT), "SYBO.Subway", "CharacterGraphicsController", "HandleRollingEnded", 0),
    E(DizzyStars, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowDizzyStars", 0),
    E(HbCrashFx, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowHoverboardCrashEffect", 0),
    E(RollDirt, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowRollDirt", 0),
    E(BushLeaves, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowBushLeaves", 0),
    E(Grind, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowGrind", 0),
    E(FlightPuff, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowFlightPuffEffect", 1),
    E(PogoFx, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowPogostickEffects", 2),
    E(BoosterFx, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowScoreBoosterEffects", 2),
    E(ReviveFx, B(F_FX), "SYBO.Subway", "CharacterEffectsController", "ShowReviveEffect", 0),
    E(ShowSneakers, B(F_PROPS), "SYBO.Subway", "CharacterPropsController", "ShowSneakers", 1),
    E(ShowMagnet, B(F_PROPS), "SYBO.Subway", "CharacterPropsController", "ShowMagnet", 1),
    E(ShowPogo, B(F_PROPS), "SYBO.Subway", "CharacterPropsController", "ShowPogostick", 1),
    E(ShowFlight, B(F_PROPS), "SYBO.Subway", "CharacterPropsController", "ShowFlight", 2),
    E(ShowHoverboard, B(F_PROPS), "SYBO.Subway", "CharacterPropsController", "ShowHoverboard", 1),
    E(PlayStripes, B(F_STRIPES), "SYBO.Subway", "CameraEffectsController", "PlaySpeedStripes", 0),
    E(StopStripes, B(F_STRIPES), "SYBO.Subway", "CameraEffectsController", "StopSpeedStripes", 0),
    E(CamImpact, B(F_SHAKE), "SYBO.Subway", "CameraController", "OnCharacterImpact", 1),
    E(AudChangeTrack, B(F_SND), "SYBO.Subway", "CharacterAudio", "HandleOnChangeTrack", 1),
    E(AudStumble, B(F_SND), "SYBO.Subway", "CharacterAudio", "HandleOnStumble", 1),
    E(AudCoin, B(F_SND), "SYBO.Subway", "CharacterAudio", "OnCoinPickedUp", 1),
    E(AudHeadstart, B(F_SND), "SYBO.Subway", "CharacterAudio", "OnHeadstartEngagedOrIncreased", 0),
    E(AudBooster, B(F_SND), "SYBO.Subway", "CharacterAudio", "OnScoreBoosterAdded", 0),
    E(TrainAudio, B(F_SND), "SYBO.Subway.Systems", "MainTrainAudioSystem", "PlayAudio", 1),
    E(PickDetect, B(F_NODET), NULL, "PickupDetector", "PickUpHitPickups", 0),
    E(AnimRoll, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnRoll", 0),
    E(AnimDive, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnDive", 0),
    E(AnimLand, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnLand", 0),
    E(AnimFall, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnFallStarted", 0),
    E(AnimDoubleJump, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnDoubleJump", 0),
    E(AnimJump, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnJump", 1),
    E(AnimLane, B(F_ANIM), "SYBO.Subway", "CharacterGraphicsController", "OnLaneChanged", 1),
    E(LaneRotation, B(F_NOROT), "SYBO.Subway", "CharacterRotationController", "OnLaneChangeStarted", 1),
    E(Hover, B(F_MACREC), "SYBO.Subway", "PowerupEventController", "TryToActivateHoverboard", 0),
    E(AbortLane, B(F_NOPULL), "SYBO.RunnerCore.Character", "CharacterMotor", "AbortLaneChange", 0),
    E(NearestLane, B(F_NOPULL), "SYBO.RunnerCore.Character", "CharacterMotor", "MoveToNearestLane", 0),
    E(getSeed, B(F_SEED), "SYBO.Subway", "RouteConfig", "get_Seed", 0),
    E(CamGameplay, B(F_CAM), "SYBO.Subway", "CameraController", "UpdateGameplay", 0),
    E(aSetPitch, B(F_AUDIO), "UnityEngine", "AudioSource", "set_pitch", 1),
    E(aPlay, B(F_AUDIO), "UnityEngine", "AudioSource", "Play", 0),
    E(CamFollow, B(F_CAMFOLLOW), NULL, "CameraGroundedModifier", "Apply", 1),
    E(CamStop, B(F_CAMSTOP), NULL, "CameraOffsetModifier", "Apply", 1),
    E(react, B(F_ZEROREACT), "SYBO.Subway", "GuardAndDogBehaviour", "get_ReactionDelay", 0),
    E(setFov, B(F_FOV), "UnityEngine", "Camera", "set_fieldOfView", 1),
    E(setTS, B(F_TS), "UnityEngine", "Time", "set_timeScale", 1),
    E(setFps, B(F_FPS), "UnityEngine", "Application", "set_targetFrameRate", 1),
};

static int hooksEnsure(int flag) {
    if (!il2cppReady()) return 0;
    if (!hookEngine()) return -1;
    int ok = 0;
    for (size_t i = 0; i < sizeof(gHK) / sizeof(gHK[0]); i++) {
        HK *h = &gHK[i];
        if (!(h->mask & B(flag)) || h->done) continue;
        void *k = h->ns ? cls(h->ns, h->cls) : clsAny(h->cls);
        void *mi = k ? il2cpp_class_get_method_from_name(k, h->m, h->np) : NULL;
        void *fn = mi ? *(void **)mi : NULL;
        if (!fn) continue;
        gMSHook(fn, h->repl, h->orig);
        if (*h->orig) { h->done = YES; ok++; }
    }
    return ok;
}

static int hooksDone(void) {
    int n = 0;
    for (size_t i = 0; i < sizeof(gHK) / sizeof(gHK[0]); i++) if (gHK[i].done) n++;
    return n;
}

#define TL_MAX 96
typedef struct { uint32_t h[TL_MAX]; int n; } TL;
static TL gTrains, gPicks;
static BOOL gTrSpin = NO, gTrScale = NO, gPkScale = NO, gHideCoins = NO, gFastReset = NO;
static float gTrRate[3] = { 0, 90, 0 };
static float gTrScaleV[3] = { 1, 1, 1 };
static float gPkScaleV[3] = { 1, 1, 1 };
static float gOneScale[3] = { 1, 1, 1 };

static void tlFree(TL *t) {
    for (int i = 0; i < t->n; i++) il2cpp_gchandle_free(t->h[i]);
    t->n = 0;
}

static void tlRefresh(TL *t, const char *cn, float pz, float lo, float hi) {
    tlFree(t);
    void *k = clsAny(cn);
    if (!k) return;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len && t->n < TL_MAX; i++) {
        void *o = items[i];
        if (!o || !objActive(o)) continue;
        void *tr = callM(mGetTr, o, NULL);
        float *p = vec3Of(tr ? callM(mGetPos, tr, NULL) : NULL);
        if (!p) continue;
        float dz = p[2] - pz;
        if (dz < lo || dz > hi) continue;
        t->h[t->n++] = il2cpp_gchandle_new(tr, 0);
    }
}

static void tlApply(TL *t, void *method, float *v) {
    for (int i = 0; i < t->n; i++) {
        void *tr = il2cpp_gchandle_get_target(t->h[i]);
        if (!tr) continue;
        void *a[1] = { v };
        callOK(method, tr, a);
    }
}

static void hideCoinsTick(void) {
    if (!gHideCoins || !il2cppReady() || !unityInit()) return;
    void *k = clsAny("CoinPickup");
    void *mSet = meth("UnityEngine", "GameObject", "SetActive", 1);
    if (!k || !mSet) return;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *o = items[i];
        if (!o || !objActive(o)) continue;
        void *go = callM(mGetGO, o, NULL);
        if (!go) continue;
        bool f = false;
        void *a[1] = { &f };
        callOK(mSet, go, a);
    }
}

static CFTimeInterval gFastCool = 0;

static void fastResetTick(void) {
    if (!gFastReset || !il2cppReady() || !unityInit()) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (now < gFastCool) return;
    void *k = clsAny("EndRunSlice");
    void *m = k ? il2cpp_class_get_method_from_name(k, "StartRun", 0) : NULL;
    if (!m) return;
    uint64_t len;
    void **items = findAll(k, &len);
    BOOL started = NO;
    for (uint64_t i = 0; i < len; i++) {
        void *o = items[i];
        if (!o || !objActive(o)) continue;
        if (callOK(m, o, NULL)) started = YES;
    }
    if (!started) return;
    gFastCool = now + 1.5;
    void *kr = clsAny("CameraRigController");
    void *mS = kr ? il2cpp_class_get_method_from_name(kr, "OnStart", 0) : NULL;
    void *mR = kr ? il2cpp_class_get_method_from_name(kr, "OnReset", 0) : NULL;
    items = findAll(kr, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *o = items[i];
        if (!o || !objActive(o)) continue;
        callOK(mS, o, NULL);
        callOK(mR, o, NULL);
    }
}

static void restoreTrains(void) {
    if (il2cppReady()) fieldRestore(clsAny("MovingTrain"), "_speed", gTrainOrig);
}

static void restorePicks(void) {
    if (il2cppReady()) fieldRestore(clsAny("PickupRotator"), "_rotationSpeed", gPickOrig);
}


static void macroFire(void *mo, MEv e) {
    if (e.type == 0) callOK(mJump, mo, NULL);
    else if (e.type == 1) callOK(il2cpp_class_get_method_from_name(kMotor, "Dive", 0), mo, NULL);
    else if (e.type == 2) {
        int v = e.arg;
        void *a[1] = { &v };
        callOK(il2cpp_class_get_method_from_name(kMotor, "ChangeLane", 1), mo, a);
    } else if (e.type == 3) callPowerup("TryToActivateHoverboard");
}

static void macroFrame(void) {
    if (!gMacRec && !gMacPlay) return;
    void *mo = cachedMotor();
    if (!mo || !mGetElapsed) return;
    float el = floatOf(callM(mGetElapsed, mo, NULL));
    if (el + 0.3f < gMacPrev) {
        if (gMacRec) { gMacN = 0; gMacSeed = gLock ? gSeed : 0; }
        if (gMacPlay) {
            gMacIdx = 0;
            gMacWait = NO;
            if (gMacSeed) { gSeed = gMacSeed; gLock = YES; applySeed(YES); }
        }
    }
    gMacPrev = el;
    gRunElapsed = el;
    if (!gMacPlay) return;
    if (gMacWait) {
        if (el < 1.0f) { gMacWait = NO; gMacIdx = 0; }
        else return;
    }
    gMacFiring = YES;
    while (gMacIdx < gMacN && gMac[gMacIdx].t <= el) macroFire(mo, gMac[gMacIdx++]);
    gMacFiring = NO;
}

static NSString *macroPath(void) {
    NSString *dir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    return [dir stringByAppendingPathComponent:@"fz_macro.bin"];
}

static NSString *macroSave(void) {
    NSMutableData *d = [NSMutableData data];
    int32_t hdr[3] = { 0x464D4143, gMacSeed, gMacN };
    [d appendBytes:hdr length:12];
    [d appendBytes:gMac length:sizeof(MEv) * gMacN];
    return [d writeToFile:macroPath() atomically:YES] ? [NSString stringWithFormat:@"Saved %d events", gMacN] : @"Save failed";
}

static NSString *macroLoad(void) {
    NSData *d = [NSData dataWithContentsOfFile:macroPath()];
    if (d.length < 12) return @"No saved macro";
    const int32_t *h = d.bytes;
    if (h[0] != 0x464D4143) return @"Bad macro file";
    int n = MIN(h[2], MAC_MAX);
    if (d.length < 12 + sizeof(MEv) * n) return @"Corrupt macro file";
    memcpy(gMac, (const char *)d.bytes + 12, sizeof(MEv) * n);
    gMacN = n;
    gMacSeed = h[1];
    gMacIdx = 0;
    return [NSString stringWithFormat:@"Loaded %d events (seed %d)", n, gMacSeed];
}


static NSString *macroToText(void) {
    NSMutableString *s = [NSMutableString string];
    for (int i = 0; i < gMacN; i++) {
        char c = 'U';
        if (gMac[i].type == 1) c = 'D';
        else if (gMac[i].type == 2) c = gMac[i].arg < 0 ? 'L' : 'R';
        else if (gMac[i].type == 3) c = 'H';
        [s appendFormat:@"%.2f:%c ", gMac[i].t, c];
    }
    return s;
}

static NSString *macroFromText(NSString *t) {
    NSMutableArray *toks = [NSMutableArray array];
    for (NSString *p in [t componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" ,;\n"]])
        if (p.length) [toks addObject:p];
    int n = 0;
    for (NSString *tk in toks) {
        NSArray *kv = [tk componentsSeparatedByString:@":"];
        if (kv.count != 2 || n >= MAC_MAX) continue;
        NSString *a = [kv[1] uppercaseString];
        if (!a.length) continue;
        unichar ch = [a characterAtIndex:0];
        uint8_t type = 0; int8_t arg = 0;
        if (ch == 'U') type = 0;
        else if (ch == 'D') type = 1;
        else if (ch == 'L') { type = 2; arg = -1; }
        else if (ch == 'R') { type = 2; arg = 1; }
        else if (ch == 'H') type = 3;
        else continue;
        gMac[n].t = [kv[0] floatValue];
        gMac[n].type = type;
        gMac[n].arg = arg;
        n++;
    }
    for (int i = 1; i < n; i++) {
        MEv e = gMac[i];
        int j = i - 1;
        while (j >= 0 && gMac[j].t > e.t) { gMac[j + 1] = gMac[j]; j--; }
        gMac[j + 1] = e;
    }
    gMacN = n;
    gMacIdx = 0;
    return [NSString stringWithFormat:@"Parsed %d events", n];
}

static BOOL gTurbo = NO;
static int gTurboRate = 120;
static char gTurboSeq[32] = "LR";
static int gTurboIdx = 0;
static float gTurboAcc = 0;

static void turboFrame(float dt) {
    if (!gTurbo) return;
    void *mo = cachedMotor();
    size_t len = strlen(gTurboSeq);
    if (!mo || !len) return;
    gTurboAcc += gTurboRate * dt;
    int n = MIN(60, (int)gTurboAcc);
    gTurboAcc -= (float)n;
    if (gTurboAcc > 1.f) gTurboAcc = 0;
    void *mDive = il2cpp_class_get_method_from_name(kMotor, "Dive", 0);
    void *mLane = il2cpp_class_get_method_from_name(kMotor, "ChangeLane", 1);
    gMacFiring = YES;
    for (int i = 0; i < n; i++) {
        char c = gTurboSeq[gTurboIdx++ % len];
        if (c == 'U') callOK(mJump, mo, NULL);
        else if (c == 'D') callOK(mDive, mo, NULL);
        else if (c == 'L' || c == 'R') {
            int v = c == 'L' ? -1 : 1;
            void *a[1] = { &v };
            callOK(mLane, mo, a);
        }
    }
    gMacFiring = NO;
}


static BOOL hookInstalled(int flag) {
    for (size_t i = 0; i < sizeof(gHK) / sizeof(gHK[0]); i++) if ((gHK[i].mask & B(flag)) && gHK[i].done) return YES;
    return NO;
}

