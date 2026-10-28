static void fieldOverride(void *k, const char *fname, float v, NSMutableDictionary *orig) {
    void *f = k ? il2cpp_class_get_field_from_name(k, fname) : NULL;
    if (!f) return;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *o = items[i];
        if (!o) continue;
        NSValue *key = [NSValue valueWithPointer:o];
        if (!orig[key]) {
            float c = 0;
            il2cpp_field_get_value(o, f, &c);
            orig[key] = @(c);
        }
        il2cpp_field_set_value(o, f, &v);
    }
}

static void fieldRestore(void *k, const char *fname, NSMutableDictionary *orig) {
    void *f = k ? il2cpp_class_get_field_from_name(k, fname) : NULL;
    if (f) {
        uint64_t len;
        void **items = findAll(k, &len);
        for (uint64_t i = 0; i < len; i++) {
            void *o = items[i];
            if (!o) continue;
            NSNumber *n = orig[[NSValue valueWithPointer:o]];
            if (!n) continue;
            float v = n.floatValue;
            il2cpp_field_set_value(o, f, &v);
        }
    }
    [orig removeAllObjects];
}

static void worldTick(void) {
    if (!gTrainOrig) gTrainOrig = [NSMutableDictionary new];
    if (!gPickOrig) gPickOrig = [NSMutableDictionary new];
    if (!gTrainOn && !gPickOn) return;
    if (!il2cppReady()) return;
    if (gTrainOn) fieldOverride(clsAny("MovingTrain"), "_speed", gTrainStop ? 0.f : gTrainSpeed, gTrainOrig);
    if (gPickOn) fieldOverride(clsAny("PickupRotator"), "_rotationSpeed", gPickSpeed, gPickOrig);
}

static BOOL parseHex(NSString *s, float out[4]) {
    NSString *t = [s stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if ([t hasPrefix:@"#"]) t = [t substringFromIndex:1];
    if (t.length != 6) return NO;
    unsigned int v = 0;
    if (![[NSScanner scannerWithString:t] scanHexInt:&v]) return NO;
    out[0] = ((v >> 16) & 255) / 255.f;
    out[1] = ((v >> 8) & 255) / 255.f;
    out[2] = (v & 255) / 255.f;
    out[3] = 1;
    return YES;
}

static BOOL gFog = NO;
static float gFogS = 450, gFogE = 680;
static float gFogCol[4] = { 0.55f, 0.75f, 1, 1 };

static void fogApply(BOOL on) {
    if (!il2cppReady()) return;
    bool b = on;
    void *a[1] = { &b };
    callOK(meth("UnityEngine", "RenderSettings", "set_fog", 1), NULL, a);
    if (!on) return;
    int mode = 1;
    void *am[1] = { &mode };
    callOK(meth("UnityEngine", "RenderSettings", "set_fogMode", 1), NULL, am);
    void *ac[1] = { gFogCol };
    callOK(meth("UnityEngine", "RenderSettings", "set_fogColor", 1), NULL, ac);
    void *as[1] = { &gFogS };
    callOK(meth("UnityEngine", "RenderSettings", "set_fogStartDistance", 1), NULL, as);
    void *ae[1] = { &gFogE };
    callOK(meth("UnityEngine", "RenderSettings", "set_fogEndDistance", 1), NULL, ae);
}

static BOOL gCamOn = NO;
static float gFov = 0, gNear = 0, gFar = 0;

static void camTick(void) {
    if (!gCamOn || !il2cppReady()) return;
    void *cam = callM(meth("UnityEngine", "Camera", "get_main", 0), NULL, NULL);
    if (!cam) return;
    if (gFov > 0) { void *a[1] = { &gFov }; callOK(meth("UnityEngine", "Camera", "set_fieldOfView", 1), cam, a); }
    if (gNear > 0) { void *a[1] = { &gNear }; callOK(meth("UnityEngine", "Camera", "set_nearClipPlane", 1), cam, a); }
    if (gFar > 0) { void *a[1] = { &gFar }; callOK(meth("UnityEngine", "Camera", "set_farClipPlane", 1), cam, a); }
}

static int gFpsLimit = 0;
static BOOL gFpsUnl = NO;

static void fpsApply(void) {
    if (gFpsLimit <= 0 || !il2cppReady()) return;
    int v = gFpsLimit;
    void *a[1] = { &v };
    callOK(meth("UnityEngine", "Application", "set_targetFrameRate", 1), NULL, a);
    if (gFpsUnl) {
        int vs = 0;
        void *a2[1] = { &vs };
        callOK(meth("UnityEngine", "QualitySettings", "set_vSyncCount", 1), NULL, a2);
    }
}

#define ESP_MAX 160
typedef struct { float x1, y1, x2, y2, depth; int cat; char name[20]; } FZDot;

static BOOL gEspOn = NO, gEspBoxes = YES, gEspLines = YES, gEspNames = YES, gEspDist = YES, gEspFill = NO, gEspSmall = YES;
static int gEspDrawn = 0;
static BOOL gEspCat[4] = { YES, YES, YES, YES };
static float gEspRange = 1500, gEspScale = 1;
static uint32_t gEspH[ESP_MAX];
static int gEspC[ESP_MAX];
static float gEspOff[ESP_MAX][3], gEspExt[ESP_MAX][3];
static char gEspName[ESP_MAX][20];
static int gEspN = 0;

static void espClear(void) {
    for (int i = 0; i < gEspN; i++) il2cpp_gchandle_free(gEspH[i]);
    gEspN = 0;
}

static void espBox(void *comp, int cat, const float pos[3], float off[3], float ext[3]) {
    static const float def[4][3] = { { 3, 3, 3 }, { 5, 5, 5 }, { 10, 14, 45 }, { 6, 10, 6 } };
    for (int k = 0; k < 3; k++) { off[k] = 0; ext[k] = def[cat][k]; }
    void *go = callM(mGetGO, comp, NULL);
    void *kRend = cls("UnityEngine", "Renderer");
    void *mGet = methP("UnityEngine", "GameObject", "GetComponentsInChildren", 2, "System.Type");
    void *mB = meth("UnityEngine", "Renderer", "get_bounds", 0);
    if (!go || !kRend || !mGet || !mB) return;
    bool inc = false;
    void *a[2] = { il2cpp_type_get_object(il2cpp_class_get_type(kRend)), &inc };
    void *arr = callM(mGet, go, a);
    if (!arr) return;
    uint64_t len = *(uint64_t *)((char *)arr + 0x18);
    void **rs = (void **)((char *)arr + 0x20);
    float mn[3] = { 1e9f, 1e9f, 1e9f }, mx[3] = { -1e9f, -1e9f, -1e9f };
    int n = 0;
    for (uint64_t i = 0; i < len && i < 24; i++) {
        void *r = rs[i];
        if (!r) continue;
        const char *cn = il2cpp_class_get_name(il2cpp_object_get_class(r));
        if (cn && (strstr(cn, "Particle") || strstr(cn, "Trail") || strstr(cn, "Line"))) continue;
        float *b = vec3Of(callM(mB, r, NULL));
        if (!b) continue;
        for (int k = 0; k < 3; k++) {
            mn[k] = fminf(mn[k], b[k] - b[3 + k]);
            mx[k] = fmaxf(mx[k], b[k] + b[3 + k]);
        }
        n++;
    }
    if (!n) return;
    for (int k = 0; k < 3; k++) {
        off[k] = (mn[k] + mx[k]) / 2 - pos[k];
        ext[k] = MAX(0.5f, MIN(300.f, (mx[k] - mn[k]) / 2));
    }
}

static void espRefresh(void) {
    espClear();
    if (!il2cppReady() || !unityInit()) return;
    void *mo = cachedMotor();
    if (!mo) return;
    float pp[3];
    if (!motorPos(mo, pp)) { dropMotor(); return; }
    const char *names[3] = { "Pickup", "MovingTrain", "GuardAndDogBehaviour" };
    for (int s = 0; s < 3; s++) {
        if (s == 0 && !(gEspCat[0] || gEspCat[1])) continue;
        if (s == 1 && !gEspCat[2]) continue;
        if (s == 2 && !gEspCat[3]) continue;
        void *k = clsAny(names[s]);
        if (!k) continue;
        uint64_t len;
        void **items = findAll(k, &len);
        for (uint64_t i = 0; i < len && gEspN < ESP_MAX; i++) {
            void *o = items[i];
            if (!o || !objActive(o)) continue;
            void *tr = callM(mGetTr, o, NULL);
            float *p = vec3Of(tr ? callM(mGetPos, tr, NULL) : NULL);
            if (!p) continue;
            float dz = p[2] - pp[2];
            if (dz < -150 || dz > gEspRange) continue;
            int cat = s + 1;
            char nm[20] = "Item";
            if (s == 0) {
                const char *cn = il2cpp_class_get_name(il2cpp_object_get_class(o));
                cat = (cn && !strcmp(cn, "CoinPickup")) ? 0 : 1;
                if (!gEspCat[cat]) continue;
                if (cn && !strcmp(cn, "CoinPickup")) snprintf(nm, 20, "Coin");
                else if (cn && !strcmp(cn, "LetterPickup")) snprintf(nm, 20, "Letter");
                else if (cn && !strcmp(cn, "PogoStickPickup")) snprintf(nm, 20, "Pogostick");
                else if (cn) {
                    snprintf(nm, 20, "%s", cn);
                    size_t L = strlen(nm);
                    if (L > 6 && !strcmp(nm + L - 6, "Pickup")) nm[L - 6] = 0;
                }
            } else if (s == 1) snprintf(nm, 20, "Train");
            else snprintf(nm, 20, "Guard");
            float pc[3] = { p[0], p[1], p[2] };
            int idx = gEspN;
            espBox(o, cat, pc, gEspOff[idx], gEspExt[idx]);
            snprintf(gEspName[idx], 20, "%s", nm);
            gEspH[idx] = il2cpp_gchandle_new(tr, 0);
            gEspC[idx] = cat;
            gEspN++;
        }
    }
}

static int espProject(FZDot *out) {
    if (!il2cppReady() || !unityInit()) return 0;
    void *mW2S = meth("UnityEngine", "Camera", "WorldToViewportPoint", 1);
    void *cam = callM(meth("UnityEngine", "Camera", "get_main", 0), NULL, NULL);
    if (!cam || !mW2S) return 0;
    int n = 0;
    for (int i = 0; i < gEspN; i++) {
        void *tr = il2cpp_gchandle_get_target(gEspH[i]);
        if (!tr) continue;
        float *p = vec3Of(callM(mGetPos, tr, NULL));
        if (!p) continue;
        float cx = p[0] + gEspOff[i][0], cy = p[1] + gEspOff[i][1], cz = p[2] + gEspOff[i][2];
        float ex = gEspExt[i][0] * gEspScale, ey = gEspExt[i][1] * gEspScale, ez = gEspExt[i][2] * gEspScale;
        float pts[4][3] = { { cx - ex, cy - ey, cz - ez }, { cx + ex, cy + ey, cz - ez },
                            { cx - ex, cy - ey, cz + ez }, { cx + ex, cy + ey, cz + ez } };
        float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f, dsum = 0;
        BOOL ok = YES;
        for (int q = 0; q < 4; q++) {
            void *a[1] = { pts[q] };
            float *s = vec3Of(callM(mW2S, cam, a));
            if (!s || s[2] <= 0) { ok = NO; break; }
            float sx = s[0], sy = 1.f - s[1];
            minx = fminf(minx, sx); maxx = fmaxf(maxx, sx);
            miny = fminf(miny, sy); maxy = fmaxf(maxy, sy);
            dsum += s[2];
        }
        if (!ok) continue;
        FZDot *d = &out[n];
        d->x1 = minx; d->y1 = miny; d->x2 = maxx; d->y2 = maxy;
        d->depth = dsum / 4.f;
        d->cat = gEspC[i];
        snprintf(d->name, 20, "%s", gEspName[i]);
        n++;
    }
    return n;
}

static BOOL gChamOn = NO, gChamX = YES;
static BOOL gChamCat[4] = { YES, YES, YES, NO };
static uint32_t gChamMat[4];
static NSMutableArray *gChamRecs;
static NSMutableSet *gChamSeen;
static NSString *gChamMsg = @"Chams: off";

static void *makeChamMaterial(const float c[3], BOOL xray) {
    void *kMat = cls("UnityEngine", "Material");
    void *mFind = meth("UnityEngine", "Shader", "Find", 1);
    void *ctor = methP("UnityEngine", "Material", ".ctor", 1, "UnityEngine.Shader");
    void *mCol = meth("UnityEngine", "Material", "set_color", 1);
    if (!kMat || !mFind || !ctor || !mCol) return NULL;
    const char *shaders[3] = { "UI/Default", "Sprites/Default", "Hidden/Internal-Colored" };
    void *sh = NULL;
    for (int i = 0; i < 3 && !sh; i++) {
        void *fa[1] = { il2cpp_string_new(shaders[i]) };
        sh = callM(mFind, NULL, fa);
    }
    if (!sh) { gChamMsg = @"Chams: no usable shader in this build"; return NULL; }
    void *mat = il2cpp_object_new(kMat);
    void *ca[1] = { sh };
    if (!mat || !callOK(ctor, mat, ca)) { gChamMsg = @"Chams: material creation failed"; return NULL; }
    float col[4] = { c[0], c[1], c[2], 1 };
    void *a1[1] = { col };
    callOK(mCol, mat, a1);
    if (xray) {
        void *setInt = methP("UnityEngine", "Material", "SetInt", 2, "System.String");
        int v = 8;
        void *a2[2] = { il2cpp_string_new("unity_GUIZTestMode"), &v };
        callOK(setInt, mat, a2);
    }
    return mat;
}

static void chamObject(void *comp, int cat) {
    void *mat = gChamMat[cat] ? il2cpp_gchandle_get_target(gChamMat[cat]) : NULL;
    if (!mat) return;
    void *go = callM(mGetGO, comp, NULL);
    void *kRend = cls("UnityEngine", "Renderer");
    void *kMat = cls("UnityEngine", "Material");
    void *mGet = methP("UnityEngine", "GameObject", "GetComponentsInChildren", 2, "System.Type");
    void *mShared = meth("UnityEngine", "Renderer", "get_sharedMaterials", 0);
    void *mSetMats = meth("UnityEngine", "Renderer", "set_materials", 1);
    if (!go || !kRend || !kMat || !mGet || !mShared || !mSetMats) return;
    bool inc = true;
    void *a[2] = { il2cpp_type_get_object(il2cpp_class_get_type(kRend)), &inc };
    void *arr = callM(mGet, go, a);
    if (!arr) return;
    uint64_t len = *(uint64_t *)((char *)arr + 0x18);
    void **rs = (void **)((char *)arr + 0x20);
    for (uint64_t i = 0; i < len; i++) {
        void *r = rs[i];
        if (!r) continue;
        const char *cn = il2cpp_class_get_name(il2cpp_object_get_class(r));
        if (cn && (strstr(cn, "Particle") || strstr(cn, "Trail") || strstr(cn, "Line"))) continue;
        NSValue *key = [NSValue valueWithPointer:r];
        if ([gChamSeen containsObject:key]) continue;
        void *orig = callM(mShared, r, NULL);
        if (!orig) continue;
        uint64_t n = *(uint64_t *)((char *)orig + 0x18);
        if (n == 0 || n > 16) continue;
        void *na = il2cpp_array_new(kMat, n);
        if (!na) continue;
        void **ne = (void **)((char *)na + 0x20);
        for (uint64_t j = 0; j < n; j++) ne[j] = mat;
        void *sa[1] = { na };
        if (!callOK(mSetMats, r, sa)) continue;
        [gChamSeen addObject:key];
        [gChamRecs addObject:@[@(il2cpp_gchandle_new(r, 0)), @(il2cpp_gchandle_new(orig, 0))]];
    }
}

static void chamRestoreAll(void) {
    BOOL ready = il2cppReady();
    void *mSetMats = ready ? meth("UnityEngine", "Renderer", "set_materials", 1) : NULL;
    for (NSArray *rec in gChamRecs) {
        uint32_t rh = [rec[0] unsignedIntValue], oh = [rec[1] unsignedIntValue];
        if (ready) {
            void *r = il2cpp_gchandle_get_target(rh);
            void *o = il2cpp_gchandle_get_target(oh);
            if (r && o) { void *a[1] = { o }; callOK(mSetMats, r, a); }
            il2cpp_gchandle_free(rh);
            il2cpp_gchandle_free(oh);
        }
    }
    [gChamRecs removeAllObjects];
    [gChamSeen removeAllObjects];
}

static void chamFreeMats(void) {
    for (int i = 0; i < 4; i++) {
        if (gChamMat[i]) il2cpp_gchandle_free(gChamMat[i]);
        gChamMat[i] = 0;
    }
}

static void chamRefresh(void) {
    if (!gChamOn) return;
    if (!gChamRecs) { gChamRecs = [NSMutableArray new]; gChamSeen = [NSMutableSet new]; }
    if (!il2cppReady() || !unityInit()) return;
    if (gChamRecs.count > 500) return;
    static const float cols[4][3] = { { 0.2f, 1, 0.3f }, { 1, 0.25f, 0.25f }, { 1, 0.85f, 0.1f }, { 0.3f, 0.55f, 1 } };
    for (int c = 0; c < 4; c++) {
        if (!gChamCat[c] || gChamMat[c]) continue;
        void *m = makeChamMaterial(cols[c], gChamX);
        if (m) gChamMat[c] = il2cpp_gchandle_new(m, 0);
    }
    void *mo = cachedMotor();
    float pp[3] = { 0, 0, 0 };
    BOOL havePos = mo && motorPos(mo, pp);
    if (gChamCat[0] && gChamMat[0]) {
        void *tr = modelTr();
        if (tr) chamObject(tr, 0);
    }
    const char *names[3] = { "MovingTrain", "Pickup", "GuardAndDogBehaviour" };
    int cats[3] = { 1, 2, 3 };
    for (int s = 0; s < 3; s++) {
        if (!gChamCat[cats[s]] || !gChamMat[cats[s]]) continue;
        void *k = clsAny(names[s]);
        if (!k) continue;
        uint64_t len;
        void **items = findAll(k, &len);
        for (uint64_t i = 0; i < len; i++) {
            void *o = items[i];
            if (!o || !objActive(o)) continue;
            if (havePos) {
                void *tr = callM(mGetTr, o, NULL);
                float *p = vec3Of(tr ? callM(mGetPos, tr, NULL) : NULL);
                if (!p || p[2] - pp[2] < -80 || p[2] - pp[2] > 700) continue;
            }
            chamObject(o, cats[s]);
            if (gChamRecs.count > 500) { gChamMsg = @"Chams: renderer limit reached"; return; }
        }
    }
    if (gChamMat[0] || gChamMat[1] || gChamMat[2] || gChamMat[3])
        gChamMsg = [NSString stringWithFormat:@"Chams: %d renderers", (int)gChamRecs.count];
}

