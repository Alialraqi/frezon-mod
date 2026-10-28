static BOOL gFreecam = NO, gCamRot = NO, gCamPosOn = NO, gCamBHop = NO, gCamAutoBend = NO, gCamBendOn = NO;
static BOOL gCamOrtho = NO, gCamAspectOn = NO, gCamDeferred = NO, gCamShakeOn = NO, gFcInit = NO;
static BOOL gCamSpin[3] = { NO, NO, NO };
static float gRCam[3] = { 0, 0, 0 }, gCamSpinSpeed = 1, gCamOff[3] = { 0, 0, 0 };
static float gBend[2] = { -0.0005f, -0.0003f }, gAutoBendStr = 0.001f, gAutoBendSpd = 0.05f;
static float gBHopStr = 20, gBHopSpd = 0.05f, gOrthoSize = 20, gAspectV = 0, gFcSpeed = 60;
static BOOL gBHopSpinReq = NO;
static float gBHopSpinDur = 1.0f;
static float gFpOff[3] = { 0, 10, -180 }, gShakeLen = 0, gShakePow = 0, gTilt = 0;
static float gFcPos[3], gFcYaw = 0, gFcPitch = 0, gJoy[2] = { 0, 0 };
static int gFcMask = 0;
static int gCamHookCalls = 0;

static void eulerQ3(float x, float y, float z, float *q) {
    float hx = x * 0.0087266463f, hy = y * 0.0087266463f, hz = z * 0.0087266463f;
    float sx = sinf(hx), cx = cosf(hx), sy = sinf(hy), cy = cosf(hy), sz = sinf(hz), cz = cosf(hz);
    float ax = cy * sx, ay = sy * cx, az = -sy * sx, aw = cy * cx;
    q[0] = ax * cz + ay * sz;
    q[1] = ay * cz - ax * sz;
    q[2] = aw * sz + az * cz;
    q[3] = aw * cz - az * sz;
}

static void camInputEnable(BOOL on) {
    void *k = clsAny("SyboInput");
    void *f = k ? il2cpp_class_get_field_from_name(k, "InputEnabled") : NULL;
    if (!f) return;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++) {
        bool v = on;
        if (items[i]) il2cpp_field_set_value(items[i], f, &v);
    }
}

static void *bendInst(void) {
    void *k = cls("SYBO.RunnerCore", "BendShaderController");
    if (!k) return NULL;
    static void *f;
    if (!f) f = il2cpp_class_get_field_from_name(k, "Instance");
    void *inst = NULL;
    if (f) il2cpp_field_static_get_value(f, &inst);
    return inst;
}

static void camBend(float x, float y) {
    void *inst = bendInst();
    static void *m;
    if (!m) {
        void *k = cls("SYBO.RunnerCore", "BendShaderController");
        m = k ? il2cpp_class_get_method_from_name(k, "ApplyBend", 2) : NULL;
    }
    if (!inst || !m) return;
    float a = x, b = y;
    void *args[2] = { &a, &b };
    callOK(m, inst, args);
}

static void camBendRestore(void) {
    void *inst = bendInst();
    float d[2] = { -0.0005f, -0.0003f };
    static void *f;
    if (!f) {
        void *k = cls("SYBO.RunnerCore", "BendShaderController");
        f = k ? il2cpp_class_get_field_from_name(k, "_defaultBend") : NULL;
    }
    if (inst && f) il2cpp_field_get_value(inst, f, d);
    camBend(d[0], d[1]);
}

static void camBendTick(void) {
    if (gCamAutoBend) {
        static float at = 0;
        at += gAutoBendSpd;
        camBend(gAutoBendStr * sinf(at), gBend[1]);
    } else if (gCamBendOn) {
        camBend(gBend[0], gBend[1]);
    }
}

static void camBendReapply(void) {
    if (gCamAutoBend || gCamBendOn) camBendTick();
    else camBendRestore();
}

static BOOL camEffect(const char *method) {
    void *k = clsAny("CameraEffectsController");
    void *m = k ? il2cpp_class_get_method_from_name(k, method, 0) : NULL;
    if (!m) return NO;
    BOOL ok = NO;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++)
        if (items[i] && objActive(items[i]) && callOK(m, items[i], NULL)) ok = YES;
    if (ok) return YES;
    void *kc = cls("SYBO.Subway", "CameraController");
    if (!kc) return NO;
    static const char *cand[4] = { "_effects", "_cameraEffects", "_effectsController", "_cameraEffectsController" };
    items = findAll(kc, &len);
    for (uint64_t i = 0; i < len; i++) {
        for (int c = 0; c < 4; c++) {
            void *f = items[i] ? il2cpp_class_get_field_from_name(kc, cand[c]) : NULL;
            if (!f) continue;
            void *eff = NULL;
            il2cpp_field_get_value(items[i], f, &eff);
            if (eff && callOK(m, eff, NULL)) ok = YES;
        }
    }
    return ok;
}

static void camResetSettings(void) {
    if (!il2cppReady()) return;
    void *cam = callM(meth("UnityEngine", "Camera", "get_main", 0), NULL, NULL);
    if (!cam) return;
    bool f = false;
    void *a1[1] = { &f };
    callOK(meth("UnityEngine", "Camera", "set_orthographic", 1), cam, a1);
    callOK(meth("UnityEngine", "Camera", "ResetAspect", 0), cam, NULL);
    int rp = -1;
    void *a2[1] = { &rp };
    callOK(meth("UnityEngine", "Camera", "set_renderingPath", 1), cam, a2);
}

static void camApplyAnnette(void *self) {
    void *ctr = callM(mGetTr, self, NULL);
    void *mSetLP = meth("UnityEngine", "Transform", "set_localPosition", 1);
    void *mSetRot = meth("UnityEngine", "Transform", "set_rotation", 1);
    if (!ctr || !mSetLP || !mSetRot) return;
    static void *fCam, *fShakeL, *fShakeP, *fTilt;
    if (!fCam) {
        void *k = il2cpp_object_get_class(self);
        fCam = il2cpp_class_get_field_from_name(k, "_camera");
        fShakeL = il2cpp_class_get_field_from_name(k, "_shakeLength");
        fShakeP = il2cpp_class_get_field_from_name(k, "_shakePower");
        fTilt = il2cpp_class_get_field_from_name(k, "_tiltOffset");
    }
    if (gCamShakeOn) {
        if (gShakeLen > 0 && fShakeL) il2cpp_field_set_value(self, fShakeL, &gShakeLen);
        if (gShakePow > 0 && fShakeP) il2cpp_field_set_value(self, fShakeP, &gShakePow);
        if (gTilt != 0 && fTilt) il2cpp_field_set_value(self, fTilt, &gTilt);
    }
    static CFTimeInterval last = 0;
    CFTimeInterval now = CACurrentMediaTime();
    float dt = last > 0 ? (float)MIN(0.05, now - last) : 0.016f;
    last = now;
    void *mo = cachedMotor();
    float pp[3], q[4];
    BOOL have = mo && motorPos(mo, pp);
    if (gFreecam) {
        if (!gFcInit) {
            float *p = vec3Of(callM(mGetPos, ctr, NULL));
            if (p) { memcpy(gFcPos, p, 12); gFcInit = YES; }
        }
        if (gFcInit) {
            float yr = gFcYaw * 0.0174532925f, pr = gFcPitch * 0.0174532925f;
            float fwd[3] = { cosf(pr) * sinf(yr), -sinf(pr), cosf(pr) * cosf(yr) };
            float rgt[3] = { cosf(yr), 0, -sinf(yr) };
            float mx = gJoy[0], my = -gJoy[1];
            float u = ((gFcMask & 1) ? 1.f : 0.f) - ((gFcMask & 2) ? 1.f : 0.f);
            for (int i = 0; i < 3; i++) gFcPos[i] += (rgt[i] * mx + fwd[i] * my) * gFcSpeed * dt;
            gFcPos[1] += u * gFcSpeed * dt;
            eulerQ3(gFcPitch, gFcYaw, 0, q);
            void *a1[1] = { gFcPos };
            void *a2[1] = { q };
            callOK(mSetLP, ctr, a1);
            callOK(mSetRot, ctr, a2);
        }
        camBend(0, 0);
        static int sk = 0;
        if ((sk++ % 10) == 0) camInputEnable(NO);
        return;
    }
    if (gCamPosOn && have) {
        float v[3] = { pp[0] + gCamOff[0], pp[1] + gCamOff[1], pp[2] + gCamOff[2] };
        void *a[1] = { v };
        callOK(mSetLP, ctr, a);
    }
    BOOL rotNow = gCamRot;
    if (gCamBHop) {
        static float bt = 0, spinT = 0;
        static BOOL spinning = NO;
        bt += gBHopSpd;
        float base = gBHopStr * sinf(bt);
        if (gBHopSpinReq && !spinning) { spinning = YES; spinT = 0; }
        gBHopSpinReq = NO;
        float angle = 0;
        if (spinning) {
            spinT += dt * (gTS ? gTSValue : 1.f);
            float dur = gBHopSpinDur > 0 ? gBHopSpinDur : 1.f;
            if (spinT >= dur) { spinT = dur; spinning = NO; }
            angle = 360.f * (0.5f - 0.5f * cosf(spinT / dur * 3.1415926535f));
        }
        gRCam[1] = base + angle;
        rotNow = YES;
    }
    for (int i = 0; i < 3; i++) if (gCamSpin[i]) { gRCam[i] += gCamSpinSpeed; rotNow = YES; }
    if (rotNow) {
        eulerQ3(gRCam[0], gRCam[1], gRCam[2], q);
        void *a[1] = { q };
        callOK(mSetRot, ctr, a);
    }
    if (gFirstPerson && have) {
        float v[3] = { pp[0] + gFpOff[0], pp[1] + gFpOff[1], pp[2] + gFpOff[2] };
        void *a[1] = { v };
        callOK(mSetLP, ctr, a);
    }
    void *cam = NULL;
    if (fCam) il2cpp_field_get_value(self, fCam, &cam);
    if (cam) {
        if (gCamOrtho) {
            bool t = true;
            void *a1[1] = { &t };
            void *a2[1] = { &gOrthoSize };
            callOK(meth("UnityEngine", "Camera", "set_orthographic", 1), cam, a1);
            callOK(meth("UnityEngine", "Camera", "set_orthographicSize", 1), cam, a2);
        }
        if (gCamAspectOn && gAspectV != 0) {
            void *a[1] = { &gAspectV };
            callOK(meth("UnityEngine", "Camera", "set_aspect", 1), cam, a);
        }
        if (gCamDeferred) {
            int rp = 3;
            void *a[1] = { &rp };
            callOK(meth("UnityEngine", "Camera", "set_renderingPath", 1), cam, a);
        }
    }
    camBendTick();
}

static void camPost(void *self) {
    if (!il2cppReady() || !unityInit()) return;
    if (gFreecam || gFirstPerson || gCamRot || gCamPosOn || gCamBHop || gCamSpin[0] || gCamSpin[1] || gCamSpin[2] ||
        gCamOrtho || gCamAspectOn || gCamDeferred || gCamAutoBend || gCamBendOn || gCamShakeOn) camApplyAnnette(self);
    if (gEspOn && gEspView) {
        FZDot dots[ESP_MAX];
        int n = espProject(dots);
        gEspDrawn = n;
        [gEspView setDots:dots count:n];
    }
}

static void h_CamGameplay(void *a, void *b, void *c, void *d) {
    o_CamGameplay(a, b, c, d);
    gCamHookCalls++;
    if (!o_CamLate) camPost(a);
}

static void h_CamLate(void *a, void *b, void *c, void *d) {
    o_CamLate(a, b, c, d);
    gCamHookCalls++;
    camPost(a);
}

static uint32_t gFpR[48];
static int gFpN = 0;

static void fpRestore(void) {
    if (!gFpN) return;
    void *m = meth("UnityEngine", "Renderer", "set_enabled", 1);
    for (int i = 0; i < gFpN; i++) {
        void *r = il2cpp_gchandle_get_target(gFpR[i]);
        bool t = true;
        void *a[1] = { &t };
        if (r) callOK(m, r, a);
        il2cpp_gchandle_free(gFpR[i]);
    }
    gFpN = 0;
}

static void fpHide(void) {
    if (!il2cppReady() || !unityInit()) return;
    void *m = meth("UnityEngine", "Renderer", "set_enabled", 1);
    if (!m) return;
    bool f = false;
    void *a[1] = { &f };
    if (gFpN > 0) {
        for (int i = 0; i < gFpN; i++) {
            void *r = il2cpp_gchandle_get_target(gFpR[i]);
            if (r) callOK(m, r, a);
        }
        return;
    }
    void *tr = modelTr();
    void *go = tr ? callM(mGetGO, tr, NULL) : NULL;
    void *kRend = cls("UnityEngine", "Renderer");
    void *mGet = methP("UnityEngine", "GameObject", "GetComponentsInChildren", 2, "System.Type");
    if (!go || !kRend || !mGet) return;
    bool inc = false;
    void *ga[2] = { il2cpp_type_get_object(il2cpp_class_get_type(kRend)), &inc };
    void *arr = callM(mGet, go, ga);
    if (!arr) return;
    uint64_t len = *(uint64_t *)((char *)arr + 0x18);
    void **rs = (void **)((char *)arr + 0x20);
    for (uint64_t i = 0; i < len && gFpN < 48; i++) {
        void *r = rs[i];
        if (!r) continue;
        const char *cn = il2cpp_class_get_name(il2cpp_object_get_class(r));
        if (cn && (strstr(cn, "Particle") || strstr(cn, "Trail") || strstr(cn, "Line"))) continue;
        callOK(m, r, a);
        gFpR[gFpN++] = il2cpp_gchandle_new(r, 0);
    }
}

