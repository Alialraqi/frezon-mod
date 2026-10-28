#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>
#include <string.h>
#include <math.h>

#if __has_include("icon_data.h")
#include "icon_data.h"
#define FZ_HAS_ICON 1
#endif

#define API(ret, name, args) static ret (*name) args;
API(void *, il2cpp_domain_get, (void))
API(void **, il2cpp_domain_get_assemblies, (void *, size_t *))
API(void *, il2cpp_assembly_get_image, (void *))
API(void *, il2cpp_thread_attach, (void *))
API(size_t, il2cpp_image_get_class_count, (void *))
API(void *, il2cpp_image_get_class, (void *, size_t))
API(const char *, il2cpp_class_get_name, (void *))
API(const char *, il2cpp_class_get_namespace, (void *))
API(void *, il2cpp_class_get_field_from_name, (void *, const char *))
API(void *, il2cpp_class_get_method_from_name, (void *, const char *, int))
API(void *, il2cpp_class_get_methods, (void *, void **))
API(const char *, il2cpp_method_get_name, (void *))
API(uint32_t, il2cpp_method_get_param_count, (void *))
API(void *, il2cpp_method_get_param, (void *, uint32_t))
API(const char *, il2cpp_type_get_name, (void *))
API(void *, il2cpp_class_get_type, (void *))
API(void *, il2cpp_type_get_object, (void *))
API(void *, il2cpp_runtime_invoke, (void *, void *, void **, void **))
API(void, il2cpp_field_get_value, (void *, void *, void *))
API(void, il2cpp_field_set_value, (void *, void *, void *))
API(void, il2cpp_field_static_get_value, (void *, void *))
API(void *, il2cpp_object_get_class, (void *))
API(void *, il2cpp_string_new, (const char *))
API(unichar *, il2cpp_string_chars, (void *))
API(int32_t, il2cpp_string_length, (void *))
API(uint32_t, il2cpp_gchandle_new, (void *, int))
API(void *, il2cpp_gchandle_get_target, (uint32_t))
API(void, il2cpp_gchandle_free, (uint32_t))
API(void *, il2cpp_object_new, (void *))
API(void *, il2cpp_array_new, (void *, uint64_t))
API(uint32_t, il2cpp_field_get_offset, (void *))

static BOOL resolveAPI(void) {
#define R(n) do { *(void **)&n = dlsym(RTLD_DEFAULT, #n); if (!n) return NO; } while (0)
    R(il2cpp_domain_get); R(il2cpp_domain_get_assemblies); R(il2cpp_assembly_get_image);
    R(il2cpp_thread_attach); R(il2cpp_image_get_class_count); R(il2cpp_image_get_class);
    R(il2cpp_class_get_name); R(il2cpp_class_get_namespace); R(il2cpp_class_get_field_from_name);
    R(il2cpp_class_get_method_from_name); R(il2cpp_class_get_methods); R(il2cpp_method_get_name);
    R(il2cpp_method_get_param_count); R(il2cpp_method_get_param); R(il2cpp_type_get_name);
    R(il2cpp_class_get_type); R(il2cpp_type_get_object); R(il2cpp_runtime_invoke);
    R(il2cpp_field_get_value); R(il2cpp_field_set_value); R(il2cpp_field_static_get_value);
    R(il2cpp_object_get_class); R(il2cpp_string_new); R(il2cpp_string_chars); R(il2cpp_string_length);
    R(il2cpp_gchandle_new); R(il2cpp_gchandle_get_target); R(il2cpp_gchandle_free);
    R(il2cpp_object_new); R(il2cpp_array_new); R(il2cpp_field_get_offset);
    return YES;
}

static void *findClass(const char *ns, const char *name) {
    size_t n = 0;
    void **asms = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &n);
    for (size_t a = 0; a < n; a++) {
        void *img = il2cpp_assembly_get_image(asms[a]);
        size_t cc = il2cpp_image_get_class_count(img);
        for (size_t i = 0; i < cc; i++) {
            void *k = il2cpp_image_get_class(img, i);
            if (!k) continue;
            const char *nm = il2cpp_class_get_name(k);
            if (!nm || strcmp(nm, name) != 0) continue;
            if (!ns) return k;
            const char *sp = il2cpp_class_get_namespace(k);
            if (sp && !strcmp(sp, ns)) return k;
        }
    }
    return NULL;
}

static BOOL il2cppReady(void) {
    if (!il2cpp_domain_get && !resolveAPI()) return NO;
    void *dom = il2cpp_domain_get();
    if (!dom) return NO;
    il2cpp_thread_attach(dom);
    return YES;
}

static NSMutableDictionary *gCache;

static void *cacheGet(NSString *k) {
    if (!gCache) gCache = [NSMutableDictionary new];
    NSValue *v = gCache[k];
    return v ? v.pointerValue : NULL;
}

static void cachePut(NSString *k, void *p) {
    if (p) gCache[k] = [NSValue valueWithPointer:p];
}

static void *cls(const char *ns, const char *name) {
    NSString *k = [NSString stringWithFormat:@"%s.%s", ns, name];
    void *c = cacheGet(k);
    if (c) return c;
    c = findClass(ns, name);
    cachePut(k, c);
    return c;
}

static void *clsAny(const char *name) {
    NSString *k = [NSString stringWithFormat:@"*.%s", name];
    void *c = cacheGet(k);
    if (c) return c;
    c = findClass(NULL, name);
    cachePut(k, c);
    return c;
}

static void *meth(const char *ns, const char *cn, const char *m, int n) {
    void *c = cls(ns, cn);
    if (!c) return NULL;
    NSString *k = [NSString stringWithFormat:@"%s.%s::%s/%d", ns, cn, m, n];
    void *mi = cacheGet(k);
    if (mi) return mi;
    mi = il2cpp_class_get_method_from_name(c, m, n);
    cachePut(k, mi);
    return mi;
}

static void *methP(const char *ns, const char *cn, const char *m, int n, const char *p0) {
    void *c = cls(ns, cn);
    if (!c) return NULL;
    NSString *k = [NSString stringWithFormat:@"%s.%s::%s/%d/%s", ns, cn, m, n, p0];
    void *mi = cacheGet(k);
    if (mi) return mi;
    void *iter = NULL, *it;
    while ((it = il2cpp_class_get_methods(c, &iter))) {
        const char *nm = il2cpp_method_get_name(it);
        if (!nm || strcmp(nm, m) != 0 || (int)il2cpp_method_get_param_count(it) != n) continue;
        const char *tn = il2cpp_type_get_name(il2cpp_method_get_param(it, 0));
        if (tn && !strcmp(tn, p0)) { mi = it; break; }
    }
    cachePut(k, mi);
    return mi;
}

static void *callM(void *m, void *obj, void **args) {
    if (!m) return NULL;
    void *exc = NULL;
    void *r = il2cpp_runtime_invoke(m, obj, args, &exc);
    return exc ? NULL : r;
}

static BOOL callOK(void *m, void *obj, void **args) {
    if (!m) return NO;
    void *exc = NULL;
    il2cpp_runtime_invoke(m, obj, args, &exc);
    return exc == NULL;
}

static BOOL boolOf(void *b) { return b && *(uint8_t *)((char *)b + 0x10); }
static float floatOf(void *b) { return b ? *(float *)((char *)b + 0x10) : 0; }
static int intOf(void *b) { return b ? *(int32_t *)((char *)b + 0x10) : 0; }
static float *vec3Of(void *b) { return b ? (float *)((char *)b + 0x10) : NULL; }

static void *kMotor;
static void *mGetTr, *mGetGO, *mActiveH, *mGetPos, *mRotate, *mIsChildOf, *mGetParent;
static void *mGetLocRot, *mSetLocRot, *mGetScale, *mSetScale, *mTeleport, *mBehOn, *mGetElapsed;
static void *mResetJump, *mSetBound, *mSetCollider, *mJump, *mFindAll;
static BOOL gUnityOk = NO;

static BOOL unityInit(void) {
    if (gUnityOk) return YES;
    kMotor = cls("SYBO.RunnerCore.Character", "CharacterMotor");
    if (!kMotor) return NO;
    mGetTr = meth("UnityEngine", "Component", "get_transform", 0);
    mGetGO = meth("UnityEngine", "Component", "get_gameObject", 0);
    mActiveH = meth("UnityEngine", "GameObject", "get_activeInHierarchy", 0);
    mGetPos = meth("UnityEngine", "Transform", "get_position", 0);
    mRotate = meth("UnityEngine", "Transform", "Rotate", 1);
    mIsChildOf = meth("UnityEngine", "Transform", "IsChildOf", 1);
    mGetParent = meth("UnityEngine", "Transform", "get_parent", 0);
    mGetLocRot = meth("UnityEngine", "Transform", "get_localRotation", 0);
    mSetLocRot = meth("UnityEngine", "Transform", "set_localRotation", 1);
    mGetScale = meth("UnityEngine", "Transform", "get_localScale", 0);
    mSetScale = meth("UnityEngine", "Transform", "set_localScale", 1);
    mBehOn = meth("UnityEngine", "Behaviour", "get_isActiveAndEnabled", 0);
    mFindAll = meth("UnityEngine", "Resources", "FindObjectsOfTypeAll", 1);
    mTeleport = il2cpp_class_get_method_from_name(kMotor, "Teleport", 1);
    mGetElapsed = il2cpp_class_get_method_from_name(kMotor, "get_ElapsedRunTime", 0);
    mResetJump = il2cpp_class_get_method_from_name(kMotor, "ResetJumpCount", 0);
    mSetBound = il2cpp_class_get_method_from_name(kMotor, "set_DisableBoundaryImpact", 1);
    mSetCollider = il2cpp_class_get_method_from_name(kMotor, "set_ColliderEnable", 1);
    mJump = il2cpp_class_get_method_from_name(kMotor, "Jump", 0);
    gUnityOk = mGetTr && mGetGO && mActiveH && mGetPos && mRotate && mIsChildOf && mGetParent
        && mGetLocRot && mSetLocRot && mGetScale && mSetScale && mFindAll && mTeleport;
    return gUnityOk;
}

static void **findAll(void *klass, uint64_t *len) {
    *len = 0;
    if (!klass || !unityInit()) return NULL;
    void *args[1] = { il2cpp_type_get_object(il2cpp_class_get_type(klass)) };
    void *arr = callM(mFindAll, NULL, args);
    if (!arr) return NULL;
    *len = *(uint64_t *)((char *)arr + 0x18);
    return (void **)((char *)arr + 0x20);
}

static BOOL objActive(void *o) {
    if (!o) return NO;
    if (mBehOn) return boolOf(callM(mBehOn, o, NULL));
    void *go = callM(mGetGO, o, NULL);
    return go && boolOf(callM(mActiveH, go, NULL));
}

static void *activeMotor(void) {
    if (!unityInit()) return NULL;
    uint64_t len;
    void **items = findAll(kMotor, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *mo = items[i];
        if (!mo) continue;
        void *go = callM(mGetGO, mo, NULL);
        if (go && boolOf(callM(mActiveH, go, NULL))) return mo;
    }
    return NULL;
}

static uint32_t gMotorHandle = 0;
static CFTimeInterval gMotorResolved = 0;

static void dropMotor(void) {
    if (gMotorHandle) il2cpp_gchandle_free(gMotorHandle);
    gMotorHandle = 0;
}

static void *cachedMotor(void) {
    void *m = gMotorHandle ? il2cpp_gchandle_get_target(gMotorHandle) : NULL;
    if (m) return m;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gMotorResolved < 1.0) return NULL;
    gMotorResolved = now;
    void *mo = activeMotor();
    if (!mo) return NULL;
    dropMotor();
    gMotorHandle = il2cpp_gchandle_new(mo, 0);
    return mo;
}

static BOOL motorPos(void *mo, float out[3]) {
    void *tr = callM(mGetTr, mo, NULL);
    float *p = vec3Of(tr ? callM(mGetPos, tr, NULL) : NULL);
    if (!p) return NO;
    out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
    return YES;
}

