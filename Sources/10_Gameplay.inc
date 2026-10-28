static BOOL gLock = NO;
static int gSeed = 12345;
static void *kRoute, *fForce;

static BOOL seedSetup(void) {
    if (fForce) return YES;
    kRoute = cls("SYBO.Subway", "RouteConfig");
    if (!kRoute) return NO;
    fForce = il2cpp_class_get_field_from_name(kRoute, "_forceSeed");
    return fForce != NULL;
}

static int applySeed(BOOL lock) {
    if (!il2cppReady() || !seedSetup()) return -1;
    uint64_t len;
    void **items = findAll(kRoute, &len);
    int done = 0;
    for (uint64_t i = 0; i < len; i++) {
        void *cfg = items[i];
        if (!cfg) continue;
        void *toggle = NULL;
        il2cpp_field_get_value(cfg, fForce, &toggle);
        if (!toggle) continue;
        void *tk = il2cpp_object_get_class(toggle);
        void *fOn = il2cpp_class_get_field_from_name(tk, "On");
        void *fVal = il2cpp_class_get_field_from_name(tk, "Value");
        if (!fOn || !fVal) continue;
        bool on = lock;
        int v = gSeed;
        il2cpp_field_set_value(toggle, fOn, &on);
        if (lock) il2cpp_field_set_value(toggle, fVal, &v);
        done++;
    }
    return done;
}

static int gHbMode = 0;
static int gHbTaps = 6;
static float gOrigDouble = -1;
static void *kInput, *fSwipeInfo, *kPowerCtl, *fHbSystem;

static BOOL hbSetup(void) {
    if (fSwipeInfo && fHbSystem) return YES;
    kInput = cls("", "SyboInput");
    kPowerCtl = cls("SYBO.Subway", "PowerupEventController");
    if (!kInput || !kPowerCtl) return NO;
    fSwipeInfo = il2cpp_class_get_field_from_name(kInput, "_swipeInfo");
    fHbSystem = il2cpp_class_get_field_from_name(kPowerCtl, "_hoverboardSystem");
    return fSwipeInfo && fHbSystem;
}

static void applyHbInput(void) {
    if (!il2cppReady() || !hbSetup()) return;
    uint64_t len;
    void **items = findAll(kInput, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *inp = items[i];
        if (!inp) continue;
        void *info = NULL;
        il2cpp_field_get_value(inp, fSwipeInfo, &info);
        if (!info) continue;
        void *fDur = il2cpp_class_get_field_from_name(il2cpp_object_get_class(info), "doubleTapDuration");
        if (!fDur) continue;
        float cur = 0;
        il2cpp_field_get_value(info, fDur, &cur);
        if (gHbMode == 0) {
            if (gOrigDouble >= 0 && cur < 0) il2cpp_field_set_value(info, fDur, &gOrigDouble);
        } else {
            if (cur >= 0 && gOrigDouble < 0) gOrigDouble = cur;
            float off = -1;
            if (cur != off) il2cpp_field_set_value(info, fDur, &off);
        }
    }
}

static BOOL callPowerup(const char *name) {
    if (!il2cppReady() || !hbSetup()) return NO;
    void *m = il2cpp_class_get_method_from_name(kPowerCtl, name, 0);
    if (!m) return NO;
    uint64_t len;
    void **items = findAll(kPowerCtl, &len);
    BOOL ok = NO;
    for (uint64_t i = 0; i < len; i++) {
        void *obj = items[i];
        if (!obj) continue;
        void *sys = NULL;
        il2cpp_field_get_value(obj, fHbSystem, &sys);
        if (!sys) continue;
        if (callOK(m, obj, NULL)) ok = YES;
    }
    return ok;
}

static void *kTheme, *fThemeInst, *mLoadCity, *mGetCity;

static void *findLoadCityString(void *klass) {
    void *iter = NULL, *m;
    while ((m = il2cpp_class_get_methods(klass, &iter))) {
        const char *nm = il2cpp_method_get_name(m);
        if (!nm || strcmp(nm, "LoadCityAsync") != 0) continue;
        if (il2cpp_method_get_param_count(m) != 1) continue;
        const char *tn = il2cpp_type_get_name(il2cpp_method_get_param(m, 0));
        if (tn && !strcmp(tn, "System.String")) return m;
    }
    return NULL;
}

static void *themeInstance(void) {
    if (!kTheme) {
        kTheme = cls("SYBO.Subway", "ThemeSystem");
        if (!kTheme) return NULL;
        fThemeInst = il2cpp_class_get_field_from_name(kTheme, "Instance");
        mLoadCity = findLoadCityString(kTheme);
        mGetCity = il2cpp_class_get_method_from_name(kTheme, "get_CurrentCity", 0);
    }
    if (!fThemeInst || !mLoadCity) return NULL;
    void *inst = NULL;
    il2cpp_field_static_get_value(fThemeInst, &inst);
    return inst;
}

static NSString *currentCity(void) {
    if (!il2cppReady()) return nil;
    void *inst = themeInstance();
    if (!inst || !mGetCity) return nil;
    void *s = callM(mGetCity, inst, NULL);
    if (!s) return nil;
    return [NSString stringWithCharacters:il2cpp_string_chars(s) length:il2cpp_string_length(s)];
}

static NSString *loadCity(const char *city) {
    if (!il2cppReady()) return @"il2cpp not ready";
    void *inst = themeInstance();
    if (!kTheme) return @"ThemeSystem not found";
    if (!mLoadCity) return @"LoadCityAsync not found";
    if (!inst) return @"Enter the game first";
    void *args[1] = { il2cpp_string_new(city) };
    return callOK(mLoadCity, inst, args) ? @"Sent, loading..." : @"Load failed (exception)";
}

static NSArray<NSString *> *cityIds(void) {
    static NSArray *a;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        a = @[@"marrakesh", @"seoul", @"amsterdam", @"barcelona", @"berlin", @"copenhagen",
              @"copenhagenviking", @"copenhagensuperrunner", @"monaco", @"zurich", @"miami",
              @"moscow", @"rio", @"sanFrancisco", @"sydney", @"theNorthPole", @"tokyo", @"bali",
              @"arabia", @"bangkok", @"buenosAires", @"cairo", @"chicago", @"hawaii", @"hongKong",
              @"iceland", @"peru", @"beijing", @"houston", @"stPetersburg", @"edinburgh",
              @"littleRock", @"miaminfl", @"cambridgehalloween", @"seattle", @"spaceStation",
              @"journeytotheEast", @"oxford", @"mumbai", @"newYork", @"newyorkplay2plant", @"paris",
              @"changan", @"veniceBeach", @"lasVegas", @"mexicocity", @"vancouverspring",
              @"vancouverautumn", @"lunarnewyear", @"subwaycity", @"greece", @"shenzhen"];
    });
    return a;
}

static NSString *teleportTo(int axis, float val, BOOL relative) {
    if (!il2cppReady()) return @"il2cpp not ready";
    if (!unityInit()) return @"Unity classes not found";
    void *mo = activeMotor();
    if (!mo) return @"No active run";
    float p[3];
    if (!motorPos(mo, p)) return @"Position unavailable";
    p[axis] = relative ? p[axis] + val : val;
    void *args[1] = { p };
    if (!callOK(mTeleport, mo, args)) return @"Teleport failed (exception)";
    return [NSString stringWithFormat:@"Moved to X %.1f  Z %.1f", p[0], p[2]];
}

static BOOL gTS = NO;
static float gTSValue = 1;

static void applyTimeScale(void) {
    if (!il2cppReady()) return;
    void *mSet = meth("UnityEngine", "Time", "set_timeScale", 1);
    void *mGet = meth("UnityEngine", "Time", "get_timeScale", 0);
    if (!mSet || !mGet) return;
    void *b = callM(mGet, NULL, NULL);
    if (!b) return;
    float cur = floatOf(b);
    if (cur == 0) return;
    if (fabsf(cur - gTSValue) < 0.0001f) return;
    float v = gTSValue;
    void *args[1] = { &v };
    callOK(mSet, NULL, args);
}

#define PHYS_N 9
static const char *kPhysNames[PHYS_N + 1] = {
    "FinalTargetSpeed", "Gravity", "JumpHeight", "AirJumpHeight",
    "GroundedTimeExtensionForForgivingJumps", "JumpDiveVelocityY", "ColliderHeight",
    "LaneChangeDuration", "MinimumTravelBeforeAllowingLaneChange", "TargetSpeedAccelerationDuration"
};
static float gPhysOrig[PHYS_N + 1];
static bool gBoundOrig = false;
static BOOL gPhysHaveOrig = NO;
static void *fMotorCfg;

static void *motorConfig(void) {
    if (!il2cppReady() || !unityInit()) return NULL;
    void *mo = activeMotor();
    if (!mo) return NULL;
    if (!fMotorCfg) fMotorCfg = il2cpp_class_get_field_from_name(kMotor, "_config");
    if (!fMotorCfg) return NULL;
    void *cfg = NULL;
    il2cpp_field_get_value(mo, fMotorCfg, &cfg);
    return cfg;
}

static void physCapture(void *cfg) {
    if (gPhysHaveOrig) return;
    void *k = il2cpp_object_get_class(cfg);
    for (int i = 0; i <= PHYS_N; i++) {
        void *f = il2cpp_class_get_field_from_name(k, kPhysNames[i]);
        if (f) il2cpp_field_get_value(cfg, f, &gPhysOrig[i]);
    }
    void *fb = il2cpp_class_get_field_from_name(k, "AllowLaneChangingTowardsBoundaries");
    if (fb) il2cpp_field_get_value(cfg, fb, &gBoundOrig);
    gPhysHaveOrig = YES;
}

static NSString *physicsOp(int op, float *vals, const BOOL *has) {
    void *cfg = motorConfig();
    if (!cfg) return @"No active run";
    void *k = il2cpp_object_get_class(cfg);
    void *f[PHYS_N + 1];
    for (int i = 0; i <= PHYS_N; i++) {
        f[i] = il2cpp_class_get_field_from_name(k, kPhysNames[i]);
        if (!f[i]) return [NSString stringWithFormat:@"Field %s not found", kPhysNames[i]];
    }
    physCapture(cfg);
    if (op == 0) {
        for (int i = 0; i < PHYS_N; i++) il2cpp_field_get_value(cfg, f[i], &vals[i]);
        return @"Read current values";
    }
    if (op == 2) {
        for (int i = 0; i <= PHYS_N; i++) il2cpp_field_set_value(cfg, f[i], &gPhysOrig[i]);
        void *fb = il2cpp_class_get_field_from_name(k, "AllowLaneChangingTowardsBoundaries");
        if (fb) il2cpp_field_set_value(cfg, fb, &gBoundOrig);
        return @"Physics reset";
    }
    int n = 0;
    for (int i = 0; i < PHYS_N; i++) {
        if (!has[i]) continue;
        float v = vals[i];
        il2cpp_field_set_value(cfg, f[i], &v);
        n++;
    }
    if (has[0]) {
        float fast = 0.0001f;
        il2cpp_field_set_value(cfg, f[PHYS_N], &fast);
    }
    return [NSString stringWithFormat:@"Applied %d value(s)", n];
}

static NSString *setBoundaryLane(BOOL on) {
    void *cfg = motorConfig();
    if (!cfg) return @"No active run";
    physCapture(cfg);
    void *fb = il2cpp_class_get_field_from_name(il2cpp_object_get_class(cfg), "AllowLaneChangingTowardsBoundaries");
    if (!fb) return @"Field not found";
    bool v = on ? true : gBoundOrig;
    il2cpp_field_set_value(cfg, fb, &v);
    return on ? @"Boundary lane change on" : @"Boundary lane change restored";
}

static BOOL gInfJump = NO, gNoBound = NO, gNoclip = NO, gAutoJump = NO;

static void motorBool(void *method, bool v) {
    if (!il2cppReady() || !unityInit() || !method) return;
    void *mo = activeMotor();
    if (!mo) return;
    void *args[1] = { &v };
    callOK(method, mo, args);
}

static void motorTick(void) {
    if (!(gInfJump || gNoBound || gNoclip || gAutoJump)) return;
    if (!il2cppReady() || !unityInit()) return;
    void *mo = cachedMotor();
    if (!mo) return;
    if (gInfJump) callOK(mResetJump, mo, NULL);
    if (gNoBound) { bool t = true; void *a[1] = { &t }; callOK(mSetBound, mo, a); }
    if (gNoclip) { bool f = false; void *a[1] = { &f }; callOK(mSetCollider, mo, a); }
    if (gAutoJump) callOK(mJump, mo, NULL);
}

static uint32_t gModelH = 0, gAnimH = 0;
static float gOrigRot[4], gOrigScale[3];
static BOOL gHaveRot = NO, gHaveScale = NO;
static CFTimeInterval gModelTry = 0;
static BOOL gSpin = NO, gScaleOn = NO;
static float gRate[3] = { 0, 360, 0 };
static float gScaleV[3] = { 1, 1, 1 };

static void modelFree(void) {
    if (gModelH) il2cpp_gchandle_free(gModelH);
    if (gAnimH) il2cpp_gchandle_free(gAnimH);
    gModelH = 0; gAnimH = 0;
    gHaveRot = NO; gHaveScale = NO;
}

static BOOL modelResolve(void) {
    if (!unityInit()) return NO;
    void *mo = cachedMotor();
    if (!mo) return NO;
    void *mt = callM(mGetTr, mo, NULL);
    if (!mt) return NO;
    void *kAnim = cls("UnityEngine", "Animator");
    if (!kAnim) return NO;
    void *best = NULL, *bestAnim = NULL;
    int bestDepth = 1000;
    uint64_t len;
    void **items = findAll(kAnim, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *an = items[i];
        if (!an || !objActive(an)) continue;
        void *at = callM(mGetTr, an, NULL);
        if (!at || at == mt) continue;
        void *a1[1] = { mt };
        if (!boolOf(callM(mIsChildOf, at, a1))) continue;
        int d = 0;
        void *pt = at;
        while (pt && pt != mt && d < 40) { pt = callM(mGetParent, pt, NULL); d++; }
        if (d < bestDepth) { bestDepth = d; best = at; bestAnim = an; }
    }
    if (!best) return NO;
    modelFree();
    float *q = vec3Of(callM(mGetLocRot, best, NULL));
    if (q) { memcpy(gOrigRot, q, 16); gHaveRot = YES; }
    float *s = vec3Of(callM(mGetScale, best, NULL));
    if (s) { memcpy(gOrigScale, s, 12); gHaveScale = YES; }
    gModelH = il2cpp_gchandle_new(best, 0);
    gAnimH = il2cpp_gchandle_new(bestAnim, 0);
    return YES;
}

static void *modelTr(void) {
    void *t = gModelH ? il2cpp_gchandle_get_target(gModelH) : NULL;
    if (t) return t;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gModelTry < 1.0) return NULL;
    gModelTry = now;
    if (!modelResolve()) return NULL;
    return gModelH ? il2cpp_gchandle_get_target(gModelH) : NULL;
}

static void spinRestore(void) {
    void *tr = gModelH ? il2cpp_gchandle_get_target(gModelH) : NULL;
    if (!tr || !gHaveRot) return;
    void *a[1] = { gOrigRot };
    callOK(mSetLocRot, tr, a);
}

static void scaleRestore(void) {
    void *tr = gModelH ? il2cpp_gchandle_get_target(gModelH) : NULL;
    if (!tr || !gHaveScale) return;
    void *a[1] = { gOrigScale };
    callOK(mSetScale, tr, a);
}

static void modelFrame(float dt) {
    if (!gSpin && !gScaleOn) return;
    if (!il2cppReady() || !unityInit()) return;
    void *tr = modelTr();
    if (!tr) return;
    if (gSpin) {
        float v[3] = { gRate[0] * dt, gRate[1] * dt, gRate[2] * dt };
        void *a[1] = { v };
        if (!callOK(mRotate, tr, a)) { modelFree(); return; }
    }
    if (gScaleOn) {
        void *a[1] = { gScaleV };
        if (!callOK(mSetScale, tr, a)) modelFree();
    }
}

static NSString *setAnimSpeed(float v) {
    if (!il2cppReady() || !unityInit()) return @"Not ready";
    gModelTry = 0;
    if (!modelTr()) return @"Model not found (start a run)";
    void *an = gAnimH ? il2cpp_gchandle_get_target(gAnimH) : NULL;
    void *m = meth("UnityEngine", "Animator", "set_speed", 1);
    void *a[1] = { &v };
    if (!an || !callOK(m, an, a)) return @"Animator speed failed";
    return [NSString stringWithFormat:@"Animation speed %.2f", v];
}

static NSMutableDictionary *gTrainOrig, *gPickOrig;
static BOOL gTrainOn = NO, gTrainStop = NO, gPickOn = NO;
static float gTrainSpeed = 0, gPickSpeed = 180;

