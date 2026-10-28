static UIColor *C(int hex, CGFloat a) {
    return [UIColor colorWithRed:((hex >> 16) & 255) / 255.0 green:((hex >> 8) & 255) / 255.0
                            blue:(hex & 255) / 255.0 alpha:a];
}

static UIColor *UC(int hex, CGFloat a) {
    CGFloat r = ((hex >> 16) & 255) / 255.0, g = ((hex >> 8) & 255) / 255.0, b = (hex & 255) / 255.0;
    CGFloat l = 0.299 * r + 0.587 * g + 0.114 * b;
    CGFloat mx = MAX(r, MAX(g, b)), mn = MIN(r, MIN(g, b));
    CGFloat sat = mx > 0 ? (mx - mn) / mx : 0;
    if (sat > 0.35 && mx > 0.5) l = 1.0;
    return [UIColor colorWithWhite:l alpha:a];
}

static UIWindow *keyWindow(void) {
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:UIWindowScene.class])
            for (UIWindow *w in ((UIWindowScene *)s).windows) if (w.isKeyWindow) return w;
    return UIApplication.sharedApplication.windows.firstObject;
}

static void dragView(UIView *v, UIPanGestureRecognizer *g) {
    UIView *sv = v.superview;
    if (!sv) return;
    CGPoint t = [g translationInView:sv];
    CGPoint c = CGPointMake(v.center.x + t.x, v.center.y + t.y);
    CGFloat hw = v.bounds.size.width / 2, hh = v.bounds.size.height / 2;
    c.x = MAX(hw, MIN(sv.bounds.size.width - hw, c.x));
    c.y = MAX(hh, MIN(sv.bounds.size.height - hh, c.y));
    v.center = c;
    [g setTranslation:CGPointZero inView:sv];
}

static UILabel *label(NSString *text, CGRect f, CGFloat size, UIFontWeight w, UIColor *col) {
    UILabel *l = [[UILabel alloc] initWithFrame:f];
    l.text = text;
    l.font = [UIFont systemFontOfSize:size weight:w];
    l.textColor = col;
    return l;
}

@interface FZOverlay : UIView
- (void)setDots:(const FZDot *)d count:(int)n;
@end

@implementation FZOverlay {
    CAShapeLayer *_stroke[4], *_fill[4], *_lines[4];
    UILabel *_labels[ESP_MAX];
}

- (instancetype)initWithFrame:(CGRect)f {
    self = [super initWithFrame:f];
    if (self) {
        self.backgroundColor = UIColor.clearColor;
        self.opaque = NO;
        self.userInteractionEnabled = NO;
        static const int kCol[4] = { 0xFFD60A, 0x40E0FF, 0xFF453A, 0x4C8DFF };
        for (int i = 0; i < 4; i++) {
            _fill[i] = [CAShapeLayer layer];
            _fill[i].fillColor = C(kCol[i], 0.16).CGColor;
            _fill[i].strokeColor = nil;
            _stroke[i] = [CAShapeLayer layer];
            _stroke[i].fillColor = nil;
            _stroke[i].strokeColor = C(kCol[i], 1).CGColor;
            _stroke[i].lineWidth = 1.5;
            _lines[i] = [CAShapeLayer layer];
            _lines[i].fillColor = nil;
            _lines[i].strokeColor = C(kCol[i], 0.8).CGColor;
            _lines[i].lineWidth = 1;
            [self.layer addSublayer:_fill[i]];
            [self.layer addSublayer:_lines[i]];
            [self.layer addSublayer:_stroke[i]];
        }
    }
    return self;
}

- (void)setDots:(const FZDot *)d count:(int)n {
    n = MAX(0, MIN(n, ESP_MAX));
    CGFloat W = self.bounds.size.width, H = self.bounds.size.height;
    UIBezierPath *ps[4], *pf[4], *pl[4];
    for (int i = 0; i < 4; i++) { ps[i] = [UIBezierPath bezierPath]; pf[i] = [UIBezierPath bezierPath]; pl[i] = [UIBezierPath bezierPath]; }
    for (int i = 0; i < n; i++) {
        int cat = MAX(0, MIN(3, d[i].cat));
        CGFloat x1 = d[i].x1 * W, y1 = d[i].y1 * H, x2 = d[i].x2 * W, y2 = d[i].y2 * H;
        CGFloat cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
        if (gEspSmall) {
            CGFloat s = MAX(10.f, MIN(46.f, 2600.f / MAX(d[i].depth, 1.f)));
            x1 = cx - s / 2; x2 = cx + s / 2; y1 = cy - s / 2; y2 = cy + s / 2;
        } else {
            if (x2 - x1 < 10) { x1 = cx - 5; x2 = cx + 5; }
            if (y2 - y1 < 10) { y1 = cy - 5; y2 = cy + 5; }
        }
        CGRect r = CGRectMake(x1, y1, x2 - x1, y2 - y1);
        if (gEspFill) [pf[cat] appendPath:[UIBezierPath bezierPathWithRect:r]];
        if (gEspBoxes) [ps[cat] appendPath:[UIBezierPath bezierPathWithRect:r]];
        if (gEspLines) { [pl[cat] moveToPoint:CGPointMake(W / 2, 0)]; [pl[cat] addLineToPoint:CGPointMake(cx, y1)]; }
        UILabel *l = _labels[i];
        if (!l) {
            l = [UILabel new];
            l.font = [UIFont systemFontOfSize:10 weight:UIFontWeightBold];
            l.textColor = UIColor.whiteColor;
            l.shadowColor = UIColor.blackColor;
            l.shadowOffset = CGSizeMake(0, 1);
            l.textAlignment = NSTextAlignmentCenter;
            l.userInteractionEnabled = NO;
            [self addSubview:l];
            _labels[i] = l;
        }
        if (gEspNames || gEspDist) {
            NSMutableString *t = [NSMutableString string];
            if (gEspNames) [t appendFormat:@"%s", d[i].name];
            if (gEspDist) [t appendFormat:@"%@%.0f", t.length ? @" " : @"", d[i].depth];
            l.text = t;
            l.frame = CGRectMake(cx - 70, y1 - 14, 140, 13);
            l.hidden = NO;
        } else {
            l.hidden = YES;
        }
    }
    for (int i = n; i < ESP_MAX; i++) if (_labels[i]) _labels[i].hidden = YES;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    for (int i = 0; i < 4; i++) {
        _stroke[i].path = ps[i].CGPath;
        _fill[i].path = pf[i].CGPath;
        _lines[i].path = pl[i].CGPath;
    }
    [CATransaction commit];
}
@end

enum {
    F_NOSTUMBLE, F_NOGRAV, F_CLIPTRAIN, F_DIRT, F_GOD1, F_GOD2, F_GOD3, F_GOD5,
    F_NOJUMP, F_NODIVE, F_NOLANE, F_NODET, F_FX, F_PROPS, F_STRIPES, F_SHAKE,
    F_SND, F_ANIM, F_NOROT, F_ZEROREACT, F_FOV, F_TS, F_FPS, F_NOPULL, F_MACREC, F_FREETP, F_SEED, F_CAM, F_AUDIO, F_CAMFOLLOW, F_CAMSTOP, F_COUNT
};
static bool fl[F_COUNT];
#define B(f) (1ULL << (f))
static BOOL gFirstPerson = NO;
static FZOverlay *gEspView;
static int gLastSeed = 0;

typedef void (*MSHookFn)(void *symbol, void *replace, void **result);
static MSHookFn gMSHook;

static BOOL hookEngine(void) {
    if (gMSHook) return YES;
    gMSHook = (MSHookFn)dlsym(RTLD_DEFAULT, "MSHookFunction");
    if (gMSHook) return YES;
    const char *paths[] = {
        "/var/jb/usr/lib/libsubstrate.dylib", "/usr/lib/libsubstrate.dylib",
        "/var/jb/Library/Frameworks/CydiaSubstrate.framework/CydiaSubstrate",
        "/Library/Frameworks/CydiaSubstrate.framework/CydiaSubstrate",
        "/var/jb/usr/lib/libellekit.dylib", "/usr/lib/libellekit.dylib"
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        void *h = dlopen(paths[i], RTLD_NOW);
        if (!h) continue;
        gMSHook = (MSHookFn)dlsym(h, "MSHookFunction");
        if (gMSHook) return YES;
    }
    return NO;
}

#define HV(N, COND) \
    static void (*o_##N)(void *, void *, void *, void *); \
    static void h_##N(void *a, void *b, void *c, void *d) { if (COND) return; o_##N(a, b, c, d); }
#define HB(N, COND, RET) \
    static bool (*o_##N)(void *, void *, void *, void *); \
    static bool h_##N(void *a, void *b, void *c, void *d) { if (COND) return RET; return o_##N(a, b, c, d); }

HV(Stumble, fl[F_NOSTUMBLE] || fl[F_GOD5])
HV(Kill, fl[F_GOD2])
HV(ApplyGravity, fl[F_NOGRAV])
HV(CancelLaneChanging, fl[F_CLIPTRAIN] || fl[F_NOPULL])
HV(AbortLane, fl[F_NOPULL])
HV(HandleRollingEnded, fl[F_DIRT])
HB(IsAutoRevive, fl[F_GOD3], true)
HB(CheckFrontal, fl[F_GOD1], false)
HB(CheckSide, fl[F_GOD1], false)
typedef struct { float t; uint8_t type; int8_t arg; } MEv;
#define MAC_MAX 6000
static MEv gMac[MAC_MAX];
static int gMacN = 0, gMacIdx = 0, gMacSeed = 0;
