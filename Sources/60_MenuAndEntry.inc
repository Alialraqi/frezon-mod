static int gTapAction = 0;

static BOOL tapActionRun(int i) {
    static const char *pw[7] = { "TryToActivateHoverboard", "HandlePogostickPickedUp", "HandleMagnetPickedUp",
        "HandleJetpackPickedUp", "HandleSuperSneakersPickedUp", "HandleDoubleScorePickedUp", "HandleMysteryPowerupPickedUp" };
    if (i >= 0 && i < 7) return callPowerup(pw[i]);
    void *mo = cachedMotor();
    if (!mo) return NO;
    BOOL ok = NO;
    gMacFiring = YES;
    if (i == 7) ok = callOK(mJump, mo, NULL);
    else if (i == 8) ok = callOK(il2cpp_class_get_method_from_name(kMotor, "Dive", 0), mo, NULL);
    else {
        int v = i == 9 ? -1 : 1;
        void *a[1] = { &v };
        ok = callOK(il2cpp_class_get_method_from_name(kMotor, "ChangeLane", 1), mo, a);
    }
    gMacFiring = NO;
    return ok;
}

static NSArray<NSString *> *tapActions(void) {
    return @[@"Hoverboard", @"Pogostick", @"Magnet", @"Jetpack", @"Super sneakers", @"Double score",
             @"Mystery box", @"Jump", @"Dive / roll", @"Move left", @"Move right"];
}

static void audioTick(void) {
    if (!gAudOn && !gAudVolOn) return;
    if (!il2cppReady() || !unityInit()) return;
    if (gAudVolOn) {
        float v = gAudVol;
        void *a[1] = { &v };
        callOK(meth("UnityEngine", "AudioListener", "set_volume", 1), NULL, a);
    }
    if (!gAudOn) return;
    void *k = cls("UnityEngine", "AudioSource");
    if (!k) return;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++) if (items[i]) audioSet(items[i]);
}

static void audioRestore(void) {
    if (!il2cppReady() || !unityInit()) return;
    void *m = meth("UnityEngine", "AudioSource", "set_pitch", 1);
    void *k = cls("UnityEngine", "AudioSource");
    if (!k || !m) return;
    float p = 1.f;
    uint64_t len;
    void **items = findAll(k, &len);
    for (uint64_t i = 0; i < len; i++) {
        void *a[1] = { &p };
        if (items[i]) callOK(m, items[i], a);
    }
}
#define KBN UIKeyboardTypeNumbersAndPunctuation
#define KBT UIKeyboardTypeDefault

static float clampf(float v, float lo, float hi) { return MAX(lo, MIN(hi, v)); }
static BOOL gFpsOn = NO, gHudList = YES;

@interface FZJoystick : UIView
@end

@implementation FZJoystick {
    UIView *_knob;
}

- (instancetype)initWithFrame:(CGRect)f {
    self = [super initWithFrame:f];
    if (self) {
        self.backgroundColor = UC(0x000000, 0.35);
        self.layer.cornerRadius = f.size.width / 2;
        self.layer.borderWidth = 2;
        self.layer.borderColor = UC(0xFFFFFF, 0.5).CGColor;
        _knob = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
        _knob.backgroundColor = UC(0xFFFFFF, 0.85);
        _knob.layer.cornerRadius = 25;
        _knob.userInteractionEnabled = NO;
        _knob.center = CGPointMake(f.size.width / 2, f.size.height / 2);
        [self addSubview:_knob];
    }
    return self;
}

- (void)moveTo:(CGPoint)p {
    CGFloat r = self.bounds.size.width / 2;
    CGFloat dx = p.x - r, dy = p.y - r, d = hypot(dx, dy);
    if (d > r) { dx *= r / d; dy *= r / d; }
    _knob.center = CGPointMake(r + dx, r + dy);
    gJoy[0] = (float)(dx / r);
    gJoy[1] = (float)(dy / r);
}

- (void)reset {
    CGFloat r = self.bounds.size.width / 2;
    _knob.center = CGPointMake(r, r);
    gJoy[0] = gJoy[1] = 0;
}

- (void)touchesBegan:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self moveTo:[t.anyObject locationInView:self]]; }
- (void)touchesMoved:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self moveTo:[t.anyObject locationInView:self]]; }
- (void)touchesEnded:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self reset]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self reset]; }
@end

@interface FZTrainingMenu : NSObject <UITextFieldDelegate, UIGestureRecognizerDelegate>
+ (instancetype)shared;
- (void)install;
@end

@implementation FZTrainingMenu {
    UIButton *_icon;
    UIView *_panel;
    UISegmentedControl *_tabs;
    NSMutableArray<UIScrollView *> *_pages;
    UIScrollView *_cur;
    CGFloat _y, _pw;
    NSMutableDictionary<NSString *, UITextField *> *_fields;
    NSMutableDictionary<NSString *, UILabel *> *_status;
    NSMutableDictionary *_blocks;
    NSMutableDictionary *_switches;
    FZOverlay *_esp;
    UILabel *_hud;
    UIView *_hudBox;
    UIButton *_tapBtn;
    UIView *_joy;
    NSArray *_fcBtns;
    UIPanGestureRecognizer *_camPan;
    BOOL _hudOn;
    CADisplayLink *_link;
    int _tapCount, _fpsFrames, _frame;
    CFTimeInterval _lastTap, _t10, _t2, _fpsLast;
    float _fps;
}

+ (instancetype)shared {
    static FZTrainingMenu *m;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ m = [FZTrainingMenu new]; });
    return m;
}

- (NSString *)fv:(NSString *)key { return _fields[key].text ?: @""; }

- (float)ff:(NSString *)key def:(float)d {
    NSString *s = [[self fv:key] stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if (!s.length) return d;
    float v = s.floatValue;
    return isfinite(v) ? v : d;
}

- (void)st:(NSString *)key set:(NSString *)t { _status[key].text = t; }

- (void)bind:(UIControl *)c event:(UIControlEvents)e block:(void (^)(id))b {
    _blocks[@((uintptr_t)(__bridge void *)c)] = [b copy];
    [c addTarget:self action:@selector(fire:) forControlEvents:e];
}

- (void)fire:(id)s {
    void (^b)(id) = _blocks[@((uintptr_t)(__bridge void *)s)];
    if (b) b(s);
}

- (void)beginPage {
    UIScrollView *s = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 92, _pw, _panel.bounds.size.height - 92)];
    s.alwaysBounceVertical = YES;
    s.keyboardDismissMode = UIScrollViewKeyboardDismissModeOnDrag;
    s.hidden = _pages.count != 0;
    [_panel addSubview:s];
    [_pages addObject:s];
    _cur = s;
    _y = 4;
}

- (void)endPage { _cur.contentSize = CGSizeMake(_pw, _y + 320); }

- (void)section:(NSString *)t {
    UIView *sep = [[UIView alloc] initWithFrame:CGRectMake(16, _y + 4, _pw - 32, 1)];
    sep.backgroundColor = UC(0xFFFFFF, 0.08);
    [_cur addSubview:sep];
    [_cur addSubview:label(t, CGRectMake(16, _y + 12, _pw - 32, 16), 12, UIFontWeightBold, UC(0x8A8A99, 1))];
    _y += 34;
}

- (UILabel *)note:(NSString *)t key:(NSString *)key {
    UILabel *l = label(t, CGRectMake(16, _y, _pw - 32, 18), 12, UIFontWeightMedium, UC(0xB5B5C3, 1));
    [_cur addSubview:l];
    if (key) _status[key] = l;
    _y += 24;
    return l;
}

- (UITextField *)makeField:(CGRect)f key:(NSString *)key text:(NSString *)text kb:(UIKeyboardType)kb {
    UITextField *t = [[UITextField alloc] initWithFrame:f];
    t.backgroundColor = UC(0x2A2A35, 1);
    t.textColor = UIColor.whiteColor;
    t.font = [UIFont systemFontOfSize:14];
    t.layer.cornerRadius = 10;
    t.layer.borderWidth = 1;
    t.layer.borderColor = UC(0xFFFFFF, 0.25).CGColor;
    t.delegate = self;
    t.leftView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
    t.leftViewMode = UITextFieldViewModeAlways;
    t.autocorrectionType = UITextAutocorrectionTypeNo;
    t.autocapitalizationType = UITextAutocapitalizationTypeNone;
    t.returnKeyType = UIReturnKeyDone;
    t.keyboardType = kb;
    t.text = text;
    t.accessibilityIdentifier = key;
    [_cur addSubview:t];
    _fields[key] = t;
    return t;
}

- (UITextField *)fld:(NSString *)title key:(NSString *)key text:(NSString *)text kb:(UIKeyboardType)kb {
    CGFloat fw = _pw - 32, lw = fw * 0.46;
    UILabel *l = label(title, CGRectMake(16, _y, lw - 6, 34), 13, UIFontWeightMedium, UC(0xB5B5C3, 1));
    l.adjustsFontSizeToFitWidth = YES;
    l.minimumScaleFactor = 0.7;
    [_cur addSubview:l];
    UITextField *t = [self makeField:CGRectMake(16 + lw, _y, fw - lw, 34) key:key text:text kb:kb];
    _y += 42;
    return t;
}

- (UIButton *)btn:(NSString *)t color:(int)hex slot:(int)i of:(int)n block:(void (^)(void))blk {
    CGFloat fw = _pw - 32, gap = 8, w = (fw - gap * (n - 1)) / n;
    UIButton *b = [UIButton buttonWithType:UIButtonTypeCustom];
    b.frame = CGRectMake(16 + i * (w + gap), _y, w, 36);
    b.backgroundColor = UC(hex, 1);
    b.layer.cornerRadius = 10;
    b.titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    b.titleLabel.adjustsFontSizeToFitWidth = YES;
    [b setTitle:t forState:UIControlStateNormal];
    CGFloat bw0 = 0;
    [b.backgroundColor getWhite:&bw0 alpha:NULL];
    [b setTitleColor:bw0 > 0.6 ? UIColor.blackColor : UIColor.whiteColor forState:UIControlStateNormal];
    [_cur addSubview:b];
    [self bind:b event:UIControlEventTouchUpInside block:^(id s) { blk(); }];
    return b;
}

- (void)endRow { _y += 44; }

- (UISwitch *)sw:(NSString *)title block:(void (^)(BOOL))blk {
    UILabel *l = label(title, CGRectMake(16, _y, _pw - 100, 31), 14, UIFontWeightMedium, UIColor.whiteColor);
    l.adjustsFontSizeToFitWidth = YES;
    l.minimumScaleFactor = 0.7;
    [_cur addSubview:l];
    UISwitch *s = [[UISwitch alloc] initWithFrame:CGRectMake(_pw - 16 - 51, _y, 51, 31)];
    _switches[title] = s;
    s.onTintColor = UC(0x1FA971, 1);
    s.thumbTintColor = UIColor.blackColor;
    s.backgroundColor = UC(0x2A2A35, 1);
    s.layer.cornerRadius = 15.5;
    [_cur addSubview:s];
    [self bind:s event:UIControlEventValueChanged block:^(id x) {
        BOOL on = ((UISwitch *)x).isOn;
        [self syncLive];
        blk(on);
        [self syncLive];
    }];
    _y += 38;
    return s;
}

- (UISegmentedControl *)seg:(NSArray *)items block:(void (^)(int))blk {
    UISegmentedControl *c = [[UISegmentedControl alloc] initWithItems:items];
    c.frame = CGRectMake(16, _y, _pw - 32, 32);
    c.selectedSegmentIndex = 0;
    c.backgroundColor = UC(0x2A2A35, 1);
    c.selectedSegmentTintColor = UC(0x3B7DDD, 1);
    [c setTitleTextAttributes:@{ NSForegroundColorAttributeName: UIColor.whiteColor } forState:UIControlStateNormal];
    [c setTitleTextAttributes:@{ NSForegroundColorAttributeName: UIColor.blackColor } forState:UIControlStateSelected];
    [_cur addSubview:c];
    [self bind:c event:UIControlEventValueChanged block:^(id x) {
        [self syncLive];
        blk((int)((UISegmentedControl *)x).selectedSegmentIndex);
    }];
    _y += 42;
    return c;
}

- (void)syncLive {
    if (!_fields.count) return;
    gTSValue = gTS ? clampf([self ff:@"ts" def:1], 0.05f, 5.f) : 1.f;
    gRate[0] = clampf([self ff:@"rx" def:0], -3600, 3600);
    gRate[1] = clampf([self ff:@"ry" def:360], -3600, 3600);
    gRate[2] = clampf([self ff:@"rz" def:0], -3600, 3600);
    gScaleV[0] = clampf([self ff:@"sx" def:1], 0.05f, 20);
    gScaleV[1] = clampf([self ff:@"sy" def:1], 0.05f, 20);
    gScaleV[2] = clampf([self ff:@"sz" def:1], 0.05f, 20);
    gFov = clampf([self ff:@"fov" def:0], 0, 170);
    gNear = clampf([self ff:@"near" def:0], 0, 5000);
    gFar = clampf([self ff:@"far" def:0], 0, 20000);
    gFogS = clampf([self ff:@"fogs" def:450], 0, 20000);
    gFogE = clampf([self ff:@"foge" def:680], 0, 20000);
    parseHex([self fv:@"fogc"], gFogCol);
    gTrainSpeed = clampf([self ff:@"trspeed" def:0], -500, 500);
    gPickSpeed = clampf([self ff:@"pkspeed" def:180], -5000, 5000);
    gTrRate[0] = clampf([self ff:@"trx" def:0], -3600, 3600);
    gTrRate[1] = clampf([self ff:@"try" def:90], -3600, 3600);
    gTrRate[2] = clampf([self ff:@"trz" def:0], -3600, 3600);
    float ts = clampf([self ff:@"trs" def:1], 0.05f, 20);
    gTrScaleV[0] = gTrScaleV[1] = gTrScaleV[2] = ts;
    float ps = clampf([self ff:@"pks" def:1], 0.05f, 20);
    gPkScaleV[0] = gPkScaleV[1] = gPkScaleV[2] = ps;
    gEspRange = clampf([self ff:@"espr" def:1500], 100, 5000);
    gEspScale = clampf([self ff:@"esps" def:1], 0.2f, 5);
    gFpsLimit = gFpsOn ? (gFpsUnl ? 1000 : (int)clampf([self ff:@"fps" def:60], 10, 240)) : 0;
    gTurboRate = (int)clampf([self ff:@"trate" def:120], 1, 2000);
    NSString *sq = [[self fv:@"tseq"] uppercaseString];
    snprintf(gTurboSeq, sizeof(gTurboSeq), "%s", sq.length ? sq.UTF8String : "LR");
    gFcSpeed = clampf([self ff:@"fcspeed" def:60], 1, 2000);
    gRCam[0] = [self ff:@"crx" def:0]; if (!gCamBHop && !gCamSpin[1]) gRCam[1] = [self ff:@"cry" def:0]; gRCam[2] = [self ff:@"crz" def:0];
    gCamSpinSpeed = clampf([self ff:@"cspin" def:1], 0.1f, 50);
    gCamOff[0] = [self ff:@"cox" def:0]; gCamOff[1] = [self ff:@"coy" def:0]; gCamOff[2] = [self ff:@"coz" def:0];
    gBHopStr = clampf([self ff:@"bhs" def:20], 0, 360);
    gBHopSpd = clampf([self ff:@"bhsp" def:0.05f], 0.001f, 1);
    gBHopSpinDur = clampf([self ff:@"bhspin" def:1], 0.2f, 10);
    gBend[0] = clampf([self ff:@"bendx" def:-0.0005f], -1, 1);
    gBend[1] = clampf([self ff:@"bendy" def:-0.0003f], -1, 1);
    gAutoBendStr = clampf([self ff:@"abs" def:0.001f], 0, 1);
    gAutoBendSpd = clampf([self ff:@"abspd" def:0.05f], 0.001f, 1);
    gOrthoSize = clampf([self ff:@"orsize" def:20], 1, 500);
    gAspectV = clampf([self ff:@"aspect" def:0], 0, 10);
    gShakeLen = clampf([self ff:@"shl" def:0], 0, 20);
    gShakePow = clampf([self ff:@"shp" def:0], 0, 500);
    gTilt = clampf([self ff:@"tilt" def:0], -300, 300);
    gFpOff[0] = [self ff:@"fpx" def:0]; gFpOff[1] = [self ff:@"fpy" def:10]; gFpOff[2] = [self ff:@"fpz" def:-180];
    gAudPitch = clampf([self ff:@"apitch" def:0.05f], 0.01f, 3);
    gAudVol = clampf([self ff:@"avol" def:1], 0, 1);
    gHbTaps = (int)clampf([self ff:@"taps" def:6], 2, 20);
}

- (void)updateHb {
    NSString *act = tapActions()[MAX(0, MIN((int)tapActions().count - 1, gTapAction))];
    if (gHbMode == 0) [self st:@"hb" set:@"Double tap: game default (hoverboard)"];
    else if (gHbMode == 1) [self st:@"hb" set:@"Double tap: disabled"];
    else [self st:@"hb" set:[NSString stringWithFormat:@"%d taps = %@", gHbTaps, act]];
}

- (void)modSwitch:(NSString *)title flag:(int)f {
    [self sw:title block:^(BOOL on) {
        fl[f] = on;
        if (!on) return;
        int r = hooksEnsure(f);
        if (r < 0) [self st:@"mods" set:@"No hook engine found (needs Substrate / ElleKit)"];
        else [self st:@"mods" set:[NSString stringWithFormat:@"Hook engine OK, hooks active: %d", hooksDone()]];
    }];
}

- (void)buildRun {
    [self beginPage];
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    [self section:@"SEED"];
    [self fld:@"Seed" key:@"seed" text:[NSString stringWithFormat:@"%d", gSeed] kb:KBN];
    [self sw:@"Lock seed" block:^(BOOL on) {
        if (!on) {
            gLock = NO;
            applySeed(NO);
            [self st:@"seed" set:@"Seed: off"];
            return;
        }
        gSeed = [self fv:@"seed"].intValue;
        gLock = YES;
        [NSUserDefaults.standardUserDefaults setInteger:gSeed forKey:@"fz_seed"];
        int n = applySeed(YES);
        if (n < 0) [self st:@"seed" set:@"Seed: il2cpp / RouteConfig not found"];
        else if (n == 0) [self st:@"seed" set:@"Seed: waiting for RouteConfig (start a run)"];
        else [self st:@"seed" set:[NSString stringWithFormat:@"Seed: LOCKED %d", gSeed]];
    }];
    [self note:@"Seed: off" key:@"seed"];

    [self section:@"DOUBLE TAP ACTION"];
    [self seg:@[@"Game default", @"Disable", @"Custom"] block:^(int i) {
        gHbMode = i;
        self->_tapCount = 0;
        applyHbInput();
        [self updateHb];
    }];
    [self fld:@"Taps needed" key:@"taps" text:@"2" kb:KBN];
    self->_tapBtn = [self btn:[NSString stringWithFormat:@"Action: %@", tapActions()[MAX(0, MIN((int)tapActions().count - 1, gTapAction))]] color:0x3B7DDD slot:0 of:1 block:^{ [self showTapActions]; }];
    [self endRow];
    [self note:@"" key:@"hb"];
    [self updateHb];

    [self section:@"TELEPORT"];
    [self fld:@"Forward distance" key:@"dist" text:[ud objectForKey:@"fz_dist"] ? [NSString stringWithFormat:@"%d", (int)[ud integerForKey:@"fz_dist"]] : @"300" kb:KBN];
    [self fld:@"Side X position" key:@"sidex" text:@"0" kb:KBN];
    [self btn:@"Go forward" color:0x8E5BD9 slot:0 of:2 block:^{
        float d = clampf([self ff:@"dist" def:0], -1700, 1700);
        [NSUserDefaults.standardUserDefaults setInteger:(NSInteger)d forKey:@"fz_dist"];
        [self st:@"tp" set:teleportTo(2, d, YES)];
    }];
    [self btn:@"Go X" color:0x8E5BD9 slot:1 of:2 block:^{
        [self st:@"tp" set:teleportTo(0, clampf([self ff:@"sidex" def:0], -200, 200), NO)];
    }];
    [self endRow];
    [self note:@"Teleport: start a run first" key:@"tp"];

    [self section:@"TIME SCALE"];
    [self fld:@"Scale (0.05 - 5)" key:@"ts" text:@"0.5" kb:KBN];
    UISwitch *tsSw = [self sw:@"Enable time scale" block:^(BOOL on) {
        gTS = on;
        [self syncLive];
        fl[F_TS] = on;
        if (on) hooksEnsure(F_TS);
        applyTimeScale();
    }];

    [self section:@"SOUND SPEED (slow-mo recording)"];
    [self fld:@"Audio pitch (0.01 - 3)" key:@"apitch" text:@"0.05" kb:KBN];
    UISwitch *audSw = [self sw:@"Sound speed (pitch of all game sounds)" block:^(BOOL on) {
        gAudOn = on;
        fl[F_AUDIO] = on;
        if (on) hooksEnsure(F_AUDIO);
        else audioRestore();
    }];
    UISwitch *audSyncSw = [self sw:@"Pitch follows time scale" block:^(BOOL on) { gAudSync = on; }];
    [self fld:@"Master volume (0 - 1)" key:@"avol" text:@"1" kb:KBN];
    [self sw:@"Apply master volume" block:^(BOOL on) {
        gAudVolOn = on;
        if (!on) {
            float v = 1.f;
            void *a[1] = { &v };
            if (il2cppReady()) callOK(meth("UnityEngine", "AudioListener", "set_volume", 1), NULL, a);
        }
    }];
    [self btn:@"Record preset 0.05" color:0x1FA971 slot:0 of:2 block:^{
        self->_fields[@"ts"].text = @"0.05";
        self->_fields[@"apitch"].text = @"0.05";
        gTS = YES;
        tsSw.on = YES;
        gAudOn = YES;
        gAudSync = NO;
        audSw.on = YES;
        audSyncSw.on = NO;
        [self syncLive];
        fl[F_TS] = true;
        fl[F_AUDIO] = true;
        hooksEnsure(F_TS);
        hooksEnsure(F_AUDIO);
        applyTimeScale();
        [self st:@"aud" set:@"Time scale 0.05 and sound pitch 0.05 are on"];
    }];
    [self btn:@"Back to normal" color:0xC24A4A slot:1 of:2 block:^{
        gTS = NO;
        tsSw.on = NO;
        gAudOn = NO;
        audSw.on = NO;
        [self syncLive];
        fl[F_TS] = false;
        fl[F_AUDIO] = false;
        applyTimeScale();
        audioRestore();
        [self st:@"aud" set:@"Time scale and sound back to normal"];
    }];
    [self endRow];
    [self note:@"Tip: record at 0.05, then speed the video x20 in CapCut" key:@"aud"];

    [self section:@"RESTART / FPS"];
    [self sw:@"Fast reset (auto restart)" block:^(BOOL on) { gFastReset = on; }];
    [self fld:@"FPS limit" key:@"fps" text:@"60" kb:KBN];
    [self sw:@"Apply FPS limit" block:^(BOOL on) {
        gFpsOn = on;
        [self syncLive];
        fl[F_FPS] = on;
        if (on) hooksEnsure(F_FPS);
        fpsApply();
    }];

    [self section:@"TURBO SWIPES"];
    [self fld:@"Swipes per second" key:@"trate" text:@"120" kb:KBN];
    [self fld:@"Sequence (L R U D)" key:@"tseq" text:@"LR" kb:KBT];
    [self sw:@"Turbo swipes (max rate)" block:^(BOOL on) {
        gTurbo = on;
        gTurboAcc = 0;
        gTurboIdx = 0;
    }];
    [self sw:@"Unlimited FPS (max display rate)" block:^(BOOL on) {
        gFpsUnl = on;
        gFpsOn = on;
        [self syncLive];
        fl[F_FPS] = on;
        if (on) hooksEnsure(F_FPS);
        fpsApply();
    }];

    [self section:@"MACRO (record / replay inputs)"];
    [self sw:@"Record inputs" block:^(BOOL on) {
        gMacRec = on;
        fl[F_MACREC] = on;
        if (on) {
            if (hooksEnsure(F_MACREC) < 0) [self st:@"mac" set:@"No hook engine: use the text macro below"];
            gMacN = 0;
            gMacPrev = 0;
            gMacSeed = gLock ? gSeed : 0;
        }
    }];
    [self sw:@"Play macro" block:^(BOOL on) {
        gMacPlay = on;
        gMacWait = on;
        gMacIdx = 0;
        gMacPrev = 0;
        if (on && gMacSeed) { gSeed = gMacSeed; gLock = YES; applySeed(YES); }
    }];
    [self btn:@"Save" color:0x4A4A5A slot:0 of:3 block:^{ [self st:@"mac" set:macroSave()]; }];
    [self btn:@"Load" color:0x4A4A5A slot:1 of:3 block:^{ [self st:@"mac" set:macroLoad()]; }];
    [self btn:@"Clear" color:0xC24A4A slot:2 of:3 block:^{ gMacN = 0; gMacIdx = 0; [self st:@"mac" set:@"Macro cleared"]; }];
    [self endRow];
    [self fld:@"Macro text (time:U/D/L/R/H)" key:@"mtext" text:@"" kb:KBT];
    [self btn:@"Events to text" color:0x3B7DDD slot:0 of:2 block:^{
        self->_fields[@"mtext"].text = macroToText();
        [self st:@"mac" set:[NSString stringWithFormat:@"%d events", gMacN]];
    }];
    [self btn:@"Text to events" color:0x3B7DDD slot:1 of:2 block:^{ [self st:@"mac" set:macroFromText([self fv:@"mtext"])]; }];
    [self endRow];
    [self note:@"Macro: idle (Record, play a run, then Play)" key:@"mac"];

    [self section:@"CONFIG"];
    [self btn:@"Save fields" color:0x4A4A5A slot:0 of:2 block:^{
        NSMutableDictionary *d = [NSMutableDictionary new];
        for (NSString *k in self->_fields) d[k] = self->_fields[k].text ?: @"";
        [NSUserDefaults.standardUserDefaults setObject:d forKey:@"fz_cfg"];
        [self st:@"cfg" set:[NSString stringWithFormat:@"Saved %d fields", (int)d.count]];
    }];
    [self btn:@"Load fields" color:0x4A4A5A slot:1 of:2 block:^{
        NSDictionary *d = [NSUserDefaults.standardUserDefaults dictionaryForKey:@"fz_cfg"];
        for (NSString *k in d) self->_fields[k].text = d[k];
        [self syncLive];
        [self st:@"cfg" set:[NSString stringWithFormat:@"Loaded %d fields", (int)d.count]];
    }];
    [self endRow];
    [self note:@"" key:@"cfg"];
    [self endPage];
}

- (void)buildPlayer {
    [self beginPage];
    [self section:@"PHYSICS (tap Read during a run)"];
    NSArray *pn = @[@"Speed", @"Gravity", @"Jump height", @"Air jump height", @"Coyote time",
                    @"Dive speed", @"Collider height", @"Lane change time", @"Lane change delay"];
    for (int i = 0; i < PHYS_N; i++) [self fld:pn[i] key:[NSString stringWithFormat:@"p%d", i] text:@"" kb:KBN];
    [self btn:@"Read current values" color:0x4A4A5A slot:0 of:1 block:^{
        float vals[PHYS_N] = { 0 };
        BOOL has[PHYS_N] = { NO };
        NSString *s = physicsOp(0, vals, has);
        if ([s hasPrefix:@"Read"])
            for (int i = 0; i < PHYS_N; i++)
                self->_fields[[NSString stringWithFormat:@"p%d", i]].text = [NSString stringWithFormat:@"%g", vals[i]];
        [self st:@"phys" set:s];
    }];
    [self endRow];
    [self sw:@"Apply custom physics" block:^(BOOL on) {
        float vals[PHYS_N] = { 0 };
        BOOL has[PHYS_N] = { NO };
        if (on) {
            for (int i = 0; i < PHYS_N; i++) {
                NSString *s = [[self fv:[NSString stringWithFormat:@"p%d", i]] stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
                if (!s.length) continue;
                float v = s.floatValue;
                if (!isfinite(v)) continue;
                vals[i] = clampf(v, -100000, 100000);
                has[i] = YES;
            }
        }
        [self st:@"phys" set:physicsOp(on ? 1 : 2, vals, has)];
    }];
    [self note:@"Physics: -" key:@"phys"];
    [self sw:@"Lane change toward boundaries" block:^(BOOL on) { [self st:@"phys" set:setBoundaryLane(on)]; }];

    [self section:@"MOVEMENT"];
    [self sw:@"Infinite jumps" block:^(BOOL on) { gInfJump = on; }];
    [self sw:@"No boundary impact" block:^(BOOL on) {
        gNoBound = on;
        if (!on) motorBool(mSetBound, false);
    }];
    [self sw:@"Auto jump (bunny hop)" block:^(BOOL on) { gAutoJump = on; }];
    [self section:@"NO PULLBACK"];
    [self modSwitch:@"No pullback (never snap to the nearest lane)" flag:F_NOPULL];

    [self section:@"MODEL"];
    [self fld:@"Spin X deg/s" key:@"rx" text:@"0" kb:KBN];
    [self fld:@"Spin Y deg/s" key:@"ry" text:@"360" kb:KBN];
    [self fld:@"Spin Z deg/s" key:@"rz" text:@"0" kb:KBN];
    [self sw:@"Spin around itself" block:^(BOOL on) {
        gSpin = on;
        if (!on) spinRestore();
    }];
    [self fld:@"Scale X" key:@"sx" text:@"1" kb:KBN];
    [self fld:@"Scale Y" key:@"sy" text:@"1" kb:KBN];
    [self fld:@"Scale Z" key:@"sz" text:@"1" kb:KBN];
    [self sw:@"Custom scale" block:^(BOOL on) {
        gScaleOn = on;
        if (!on) scaleRestore();
    }];
    [self fld:@"Animation speed" key:@"anim" text:@"1" kb:KBN];
    [self sw:@"Custom animation speed" block:^(BOOL on) {
        [self st:@"model" set:setAnimSpeed(on ? [self ff:@"anim" def:1] : 1.f)];
    }];
    [self note:@"" key:@"model"];

    [self section:@"POWERUPS"];
    NSArray *pw = @[@[@"Magnet", @"HandleMagnetPickedUp"], @[@"Jetpack", @"HandleJetpackPickedUp"],
                    @[@"Super sneakers", @"HandleSuperSneakersPickedUp"], @[@"Double score", @"HandleDoubleScorePickedUp"],
                    @[@"Mystery", @"HandleMysteryPowerupPickedUp"], @[@"Pogostick", @"HandlePogostickPickedUp"],
                    @[@"Hoverboard", @"TryToActivateHoverboard"]];
    for (NSUInteger i = 0; i < pw.count; i += 2) {
        for (int j = 0; j < 2 && i + j < pw.count; j++) {
            NSString *title = pw[i + j][0], *mn = pw[i + j][1];
            [self btn:title color:0x6B5BD9 slot:j of:2 block:^{
                BOOL ok = callPowerup(mn.UTF8String);
                [self st:@"pw" set:[title stringByAppendingString:ok ? @": granted" : @": not available"]];
            }];
        }
        [self endRow];
    }
    [self note:@"" key:@"pw"];
    [self endPage];
}

- (void)buildWorld {
    [self beginPage];
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    [self section:@"MAP CHANGER"];
    [self makeField:CGRectMake(16, _y, _pw - 32 - 74, 38) key:@"city" text:[ud stringForKey:@"fz_city"] ?: @"tokyo" kb:KBT];
    UIButton *lb = [UIButton buttonWithType:UIButtonTypeCustom];
    lb.frame = CGRectMake(_pw - 16 - 66, _y, 66, 38);
    lb.backgroundColor = UC(0x4A4A5A, 1);
    lb.layer.cornerRadius = 10;
    lb.titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    [lb setTitle:@"List" forState:UIControlStateNormal];
    [lb setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [_cur addSubview:lb];
    [self bind:lb event:UIControlEventTouchUpInside block:^(id s) { [self showCities]; }];
    _y += 46;
    [self btn:@"Apply map" color:0x3B7DDD slot:0 of:1 block:^{
        NSString *city = [[self fv:@"city"] stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
        if (!city.length) { [self st:@"map" set:@"Enter a map name"]; return; }
        [NSUserDefaults.standardUserDefaults setObject:city forKey:@"fz_city"];
        [self st:@"map" set:loadCity(city.UTF8String)];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            NSString *c = currentCity();
            if (c.length) [self st:@"map" set:[@"Current map: " stringByAppendingString:c]];
        });
    }];
    [self endRow];
    [self note:@"Map: -" key:@"map"];

    [self section:@"TRAINS"];
    [self fld:@"Speed override" key:@"trspeed" text:@"0" kb:KBN];
    [self sw:@"Override moving train speed" block:^(BOOL on) {
        gTrainOn = on;
        if (!on) restoreTrains();
    }];
    [self fld:@"Spin X deg/s" key:@"trx" text:@"0" kb:KBN];
    [self fld:@"Spin Y deg/s" key:@"try" text:@"90" kb:KBN];
    [self fld:@"Spin Z deg/s" key:@"trz" text:@"0" kb:KBN];
    [self sw:@"Spin trains" block:^(BOOL on) { gTrSpin = on; }];
    [self fld:@"Train scale" key:@"trs" text:@"1" kb:KBN];
    [self sw:@"Scale trains" block:^(BOOL on) {
        gTrScale = on;
        if (!on) tlApply(&gTrains, mSetScale, gOneScale);
    }];

    [self section:@"PICKUPS"];
    [self fld:@"Rotation speed" key:@"pkspeed" text:@"180" kb:KBN];
    [self sw:@"Override pickup rotation" block:^(BOOL on) {
        gPickOn = on;
        if (!on) restorePicks();
    }];
    [self fld:@"Pickup scale" key:@"pks" text:@"1" kb:KBN];
    [self sw:@"Scale pickups" block:^(BOOL on) {
        gPkScale = on;
        if (!on) tlApply(&gPicks, mSetScale, gOneScale);
    }];
    [self sw:@"Hide all coins (no-coin practice)" block:^(BOOL on) { gHideCoins = on; }];

    [self section:@"FOG"];
    [self fld:@"Start distance" key:@"fogs" text:@"450" kb:KBN];
    [self fld:@"End distance" key:@"foge" text:@"680" kb:KBN];
    [self fld:@"Color (hex RRGGBB)" key:@"fogc" text:@"8CBFFF" kb:KBT];
    [self sw:@"Enable fog" block:^(BOOL on) {
        gFog = on;
        fogApply(on);
    }];

    [self section:@"CAMERA (0 = unchanged)"];
    [self fld:@"Field of view" key:@"fov" text:@"0" kb:KBN];
    [self fld:@"Near clip" key:@"near" text:@"0" kb:KBN];
    [self fld:@"Far clip" key:@"far" text:@"0" kb:KBN];
    [self sw:@"Apply camera values" block:^(BOOL on) {
        gCamOn = on;
        fl[F_FOV] = on;
        if (on) hooksEnsure(F_FOV);
    }];

    [self section:@"PRESET: Nearest Eurfex"];
    [self btn:@"Apply preset" color:0x1FA971 slot:0 of:2 block:^{ [self applyEurfex]; }];
    [self btn:@"Camera off" color:0xC24A4A slot:1 of:2 block:^{ [self clearCamSwitches]; }];
    [self endRow];
    [self section:@"FREECAM (needs hooks)"];
    [self fld:@"Speed (units/s)" key:@"fcspeed" text:@"60" kb:KBN];
    [self sw:@"Freecam (joystick = move, drag = look)" block:^(BOOL on) {
        gFreecam = on;
        gFcInit = NO;
        gFcMask = 0;
        gJoy[0] = gJoy[1] = 0;
        self->_joy.hidden = !on;
        for (UIView *v in self->_fcBtns) v.hidden = !on;
        self->_camPan.enabled = on;
        if (on) {
            int r = hooksEnsure(F_CAM);
            [self st:@"cam" set:r < 0 ? @"No hook engine (needs Substrate / ElleKit)" : @"Freecam on: game input is off"];
        } else if (il2cppReady()) {
            camInputEnable(YES);
            camBendReapply();
            [self st:@"cam" set:@"Freecam off"];
        }
    }];

    [self section:@"CAMERA ROTATION / POSITION (needs hooks)"];
    [self fld:@"Rotation X" key:@"crx" text:@"0" kb:KBN];
    [self fld:@"Rotation Y" key:@"cry" text:@"0" kb:KBN];
    [self fld:@"Rotation Z" key:@"crz" text:@"0" kb:KBN];
    [self sw:@"Custom camera rotation" block:^(BOOL on) { gCamRot = on; if (on) hooksEnsure(F_CAM); }];
    [self fld:@"Spin speed" key:@"cspin" text:@"1" kb:KBN];
    [self sw:@"Spin X" block:^(BOOL on) { gCamSpin[0] = on; if (on) hooksEnsure(F_CAM); }];
    [self sw:@"Spin Y" block:^(BOOL on) { gCamSpin[1] = on; if (on) hooksEnsure(F_CAM); }];
    [self sw:@"Spin Z" block:^(BOOL on) { gCamSpin[2] = on; if (on) hooksEnsure(F_CAM); }];
    [self fld:@"Offset X" key:@"cox" text:@"0" kb:KBN];
    [self fld:@"Offset Y" key:@"coy" text:@"0" kb:KBN];
    [self fld:@"Offset Z" key:@"coz" text:@"0" kb:KBN];
    [self sw:@"Camera follows player with this offset" block:^(BOOL on) { gCamPosOn = on; if (on) hooksEnsure(F_CAM); }];
    [self seg:@[@"2D off", @"2D right", @"2D left"] block:^(int i) {
        gCamRot = i != 0;
        gCamPosOn = i != 0;
        float ry = i == 1 ? 270 : (i == 2 ? 90 : 0), ox = i == 1 ? 90 : (i == 2 ? -90 : 0), oy = i ? 20 : 0, oz = i ? -165 : 0;
        self->_fields[@"crx"].text = @"0";
        self->_fields[@"cry"].text = [NSString stringWithFormat:@"%g", ry];
        self->_fields[@"crz"].text = @"0";
        self->_fields[@"cox"].text = [NSString stringWithFormat:@"%g", ox];
        self->_fields[@"coy"].text = [NSString stringWithFormat:@"%g", oy];
        self->_fields[@"coz"].text = [NSString stringWithFormat:@"%g", oz];
        [self syncLive];
        if (i) hooksEnsure(F_CAM);
    }];
    [self fld:@"Strafe amplitude" key:@"bhs" text:@"20" kb:KBN];
    [self fld:@"Strafe speed" key:@"bhsp" text:@"0.05" kb:KBN];
    [self sw:@"Camera strafing (bhop)" block:^(BOOL on) { gCamBHop = on; if (on) hooksEnsure(F_CAM); }];
    [self fld:@"Spin duration (s)" key:@"bhspin" text:@"1" kb:KBN];
    [self btn:@"Spin 360 now" color:0x3B7DDD slot:0 of:1 block:^{ gBHopSpinReq = YES; }];
    [self endRow];

    [self section:@"FIRST PERSON (needs hooks)"];
    [self fld:@"Offset X" key:@"fpx" text:@"0" kb:KBN];
    [self fld:@"Offset Y" key:@"fpy" text:@"10" kb:KBN];
    [self fld:@"Offset Z" key:@"fpz" text:@"-180" kb:KBN];
    [self sw:@"First person view" block:^(BOOL on) {
        gFirstPerson = on;
        if (on) {
            int r = hooksEnsure(F_CAM);
            [self st:@"cam" set:r < 0 ? @"No hook engine (needs Substrate / ElleKit)" : @"First person on"];
        } else {
            fpRestore();
            [self st:@"cam" set:@"First person off"];
        }
    }];

    [self section:@"WORLD BEND / LENS (needs hooks)"];
    [self fld:@"Bend X" key:@"bendx" text:@"-0.0005" kb:KBN];
    [self fld:@"Bend Y" key:@"bendy" text:@"-0.0003" kb:KBN];
    [self sw:@"Custom world bend" block:^(BOOL on) {
        gCamBendOn = on;
        if (on) hooksEnsure(F_CAM);
        else if (il2cppReady()) camBendReapply();
    }];
    [self fld:@"Auto bend strength" key:@"abs" text:@"0.001" kb:KBN];
    [self fld:@"Auto bend speed" key:@"abspd" text:@"0.05" kb:KBN];
    [self sw:@"Auto bend (swaying curve)" block:^(BOOL on) {
        gCamAutoBend = on;
        if (on) hooksEnsure(F_CAM);
        else if (il2cppReady()) camBendReapply();
    }];
    [self fld:@"Orthographic size" key:@"orsize" text:@"20" kb:KBN];
    [self sw:@"Orthographic camera" block:^(BOOL on) {
        gCamOrtho = on;
        if (on) hooksEnsure(F_CAM);
        else camResetSettings();
    }];
    [self fld:@"Aspect ratio (0 = default)" key:@"aspect" text:@"0" kb:KBN];
    [self sw:@"Custom aspect ratio" block:^(BOOL on) {
        gCamAspectOn = on;
        if (on) hooksEnsure(F_CAM);
        else camResetSettings();
    }];
    [self sw:@"Deferred rendering path" block:^(BOOL on) {
        gCamDeferred = on;
        if (on) hooksEnsure(F_CAM);
        else camResetSettings();
    }];

    [self section:@"CAMERA SHAKE (needs hooks)"];
    [self fld:@"Shake duration" key:@"shl" text:@"0" kb:KBN];
    [self fld:@"Shake power" key:@"shp" text:@"0" kb:KBN];
    [self fld:@"Tilt" key:@"tilt" text:@"0" kb:KBN];
    [self sw:@"Apply shake values" block:^(BOOL on) { gCamShakeOn = on; if (on) hooksEnsure(F_CAM); }];
    [self btn:@"Revive effect" color:0x4A4A5A slot:0 of:3 block:^{ [self st:@"cam" set:camEffect("PlayReviveEffect") ? @"Revive effect played" : @"Not available"]; }];
    [self btn:@"Stripes on" color:0x4A4A5A slot:1 of:3 block:^{ [self st:@"cam" set:camEffect("PlaySpeedStripes") ? @"Speed stripes on" : @"Not available"]; }];
    [self btn:@"Stripes off" color:0x4A4A5A slot:2 of:3 block:^{ [self st:@"cam" set:camEffect("StopSpeedStripes") ? @"Speed stripes off" : @"Not available"]; }];
    [self endRow];
    [self note:@"" key:@"cam"];
    [self endPage];
}

- (void)buildVisual {
    [self beginPage];
    [self section:@"ESP (markers, practice only)"];
    [self sw:@"ESP" block:^(BOOL on) {
        gEspOn = on;
        self->_esp.hidden = !on;
        if (on) { hooksEnsure(F_CAM); espRefresh(); }
        else { [self->_esp setDots:NULL count:0]; espClear(); }
    }];
    [self sw:@"Coins" block:^(BOOL on) { gEspCat[0] = on; }].on = YES;
    [self sw:@"Pickups / powerups" block:^(BOOL on) { gEspCat[1] = on; }].on = YES;
    [self sw:@"Trains" block:^(BOOL on) { gEspCat[2] = on; }].on = YES;
    [self sw:@"Police" block:^(BOOL on) { gEspCat[3] = on; }].on = YES;
    [self sw:@"Small boxes (fixed size)" block:^(BOOL on) { gEspSmall = on; }].on = YES;
    [self sw:@"Boxes" block:^(BOOL on) { gEspBoxes = on; }].on = YES;
    [self sw:@"Filled boxes" block:^(BOOL on) { gEspFill = on; }];
    [self sw:@"Lines (top of screen)" block:^(BOOL on) { gEspLines = on; }].on = YES;
    [self sw:@"Names" block:^(BOOL on) { gEspNames = on; }].on = YES;
    [self sw:@"Distance" block:^(BOOL on) { gEspDist = on; }].on = YES;
    [self fld:@"Range ahead" key:@"espr" text:@"1500" kb:KBN];
    [self fld:@"Box size multiplier" key:@"esps" text:@"1" kb:KBN];
    [self note:@"ESP: off" key:@"esp"];

    [self section:@"CHAMS (solid color, experimental)"];
    [self sw:@"Chams" block:^(BOOL on) {
        gChamOn = on;
        if (!gChamRecs) { gChamRecs = [NSMutableArray new]; gChamSeen = [NSMutableSet new]; }
        if (!on) {
            chamRestoreAll();
            chamFreeMats();
            [self st:@"cham" set:@"Chams: off"];
        }
    }];
    [self sw:@"See through walls" block:^(BOOL on) {
        gChamX = on;
        chamRestoreAll();
        chamFreeMats();
    }].on = YES;
    [self sw:@"Player" block:^(BOOL on) { gChamCat[0] = on; chamRestoreAll(); }].on = YES;
    [self sw:@"Trains" block:^(BOOL on) { gChamCat[1] = on; chamRestoreAll(); }].on = YES;
    [self sw:@"Pickups" block:^(BOOL on) { gChamCat[2] = on; chamRestoreAll(); }].on = YES;
    [self sw:@"Police" block:^(BOOL on) { gChamCat[3] = on; chamRestoreAll(); }];
    [self note:@"Chams: off" key:@"cham"];

    [self section:@"HUD"];
    [self sw:@"HUD (FPS / run / position / seed)" block:^(BOOL on) { [self setHud:on]; }];
    [self sw:@"List active features under HUD" block:^(BOOL on) { gHudList = on; }].on = YES;
    [self endPage];
}

- (void)buildMods {
    [self beginPage];
    [self note:@"Hooks need Substrate / ElleKit (jailbreak)" key:@"mods"];
    [self section:@"GOD MODE"];
    [self modSwitch:@"God V1 (no impacts)" flag:F_GOD1];
    [self modSwitch:@"God V2 (cannot die)" flag:F_GOD2];
    [self modSwitch:@"God V3 (auto revive)" flag:F_GOD3];
    [self modSwitch:@"God V5 (edges + no stumble)" flag:F_GOD5];
    [self section:@"MOVEMENT"];
    [self modSwitch:@"No stumble" flag:F_NOSTUMBLE];
    [self modSwitch:@"No gravity" flag:F_NOGRAV];
    [self modSwitch:@"Clip into trains" flag:F_CLIPTRAIN];
    [self modSwitch:@"Dirt trail glitch" flag:F_DIRT];
    [self section:@"CHALLENGES"];
    [self modSwitch:@"No jump" flag:F_NOJUMP];
    [self modSwitch:@"No dive / roll" flag:F_NODIVE];
    [self modSwitch:@"One lane (no lane change)" flag:F_NOLANE];
    [self modSwitch:@"No pickups at all" flag:F_NODET];
    [self section:@"VISUALS OFF"];
    [self modSwitch:@"No particle effects" flag:F_FX];
    [self modSwitch:@"Invisible props" flag:F_PROPS];
    [self modSwitch:@"No speed stripes" flag:F_STRIPES];
    [self modSwitch:@"No camera shake" flag:F_SHAKE];
    [self modSwitch:@"Camera follow" flag:F_CAMFOLLOW];
    [self modSwitch:@"Stop camera" flag:F_CAMSTOP];
    [self modSwitch:@"No character animations" flag:F_ANIM];
    [self modSwitch:@"No lane rotation" flag:F_NOROT];
    [self section:@"AUDIO / POLICE"];
    [self modSwitch:@"Mute character and train sounds" flag:F_SND];
    [self modSwitch:@"Police: 0 reaction delay" flag:F_ZEROREACT];
    [self endPage];
}

- (void)setSwitch:(NSString *)title on:(BOOL)on {
    UISwitch *s = _switches[title];
    if (!s || s.isOn == on) return;
    s.on = on;
    [s sendActionsForControlEvents:UIControlEventValueChanged];
}

- (void)applyEurfex {
    NSDictionary *f = @{ @"fov": @"135", @"cox": @"0", @"coy": @"4", @"coz": @"0",
                         @"crx": @"0", @"cry": @"25", @"crz": @"0",
                         @"bendx": @"0", @"bendy": @"0.004", @"bhs": @"6", @"bhsp": @"0.005",
                         @"ts": @"0.05", @"apitch": @"0.05" };
    for (NSString *k in f) _fields[k].text = f[k];
    [self syncLive];
    NSArray *on = @[@"Apply camera values", @"First person view", @"Camera follows player with this offset",
                    @"Custom camera rotation", @"Custom world bend", @"Camera strafing (bhop)",
                    @"Enable time scale", @"Sound speed (pitch of all game sounds)"];
    for (NSString *t in on) [self setSwitch:t on:YES];
    [self st:@"cam" set:@"Preset applied"];
}

- (void)clearCamSwitches {
    NSArray *off = @[@"Apply camera values", @"First person view", @"Camera follows player with this offset",
                     @"Custom camera rotation", @"Custom world bend", @"Camera strafing (bhop)", @"Freecam (joystick = move, drag = look)"];
    for (NSString *t in off) [self setSwitch:t on:NO];
    [self st:@"cam" set:@"Camera features off"];
}

- (void)showCities {
    [_panel endEditing:YES];
    UIViewController *vc = keyWindow().rootViewController;
    while (vc.presentedViewController) vc = vc.presentedViewController;
    UIAlertController *a = [UIAlertController alertControllerWithTitle:@"Maps" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    for (NSString *c in cityIds()) {
        [a addAction:[UIAlertAction actionWithTitle:c style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) {
            self->_fields[@"city"].text = c;
        }]];
    }
    [a addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    a.popoverPresentationController.sourceView = _panel;
    a.popoverPresentationController.sourceRect = _panel.bounds;
    [vc presentViewController:a animated:YES completion:nil];
}

- (void)setHud:(BOOL)on {
    UIWindow *w = keyWindow();
    _hudOn = on;
    if (on && w) {
        if (!_hudBox) {
            _hudBox = [[UIView alloc] initWithFrame:CGRectMake(10, w.safeAreaInsets.top + 4, 224, 60)];
            _hudBox.backgroundColor = UC(0x000000, 0.62);
            _hudBox.layer.cornerRadius = 10;
            _hudBox.layer.borderWidth = 1;
            _hudBox.layer.borderColor = UC(0xFFFFFF, 0.35).CGColor;
            _hudBox.userInteractionEnabled = NO;
            _hud = [[UILabel alloc] initWithFrame:CGRectMake(8, 6, 208, 48)];
            _hud.numberOfLines = 0;
            _hud.font = [UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightSemibold];
            _hud.textColor = UIColor.whiteColor;
            [_hudBox addSubview:_hud];
        }
        [w addSubview:_hudBox];
        hooksEnsure(F_SEED);
    } else {
        [_hudBox removeFromSuperview];
    }
}

- (NSString *)activeList {
    static const char *names[F_COUNT] = {
        "No stumble", "No gravity", "Clip trains", "Dirt glitch", "God V1", "God V2", "God V3", "God V5",
        "No jump", "No dive", "One lane", "No pickups", "No particles", "Invisible props", "No speed stripes",
        "No cam shake", "Muted", "No anims", "No lane rotation", "Police 0 delay", NULL, NULL, NULL,
        "No pullback", NULL, NULL, NULL, NULL
    };
    NSMutableArray *a = [NSMutableArray array];
    if (gLock) [a addObject:[NSString stringWithFormat:@"Seed lock %d", gSeed]];
    if (gHbMode == 1) [a addObject:@"Hoverboard tap off"];
    if (gHbMode == 2) [a addObject:[NSString stringWithFormat:@"Hoverboard %d taps", gHbTaps]];
    if (gTS) [a addObject:[NSString stringWithFormat:@"Time x%.2f", gTSValue]];
    if (gFpsOn) [a addObject:gFpsUnl ? @"Unlimited FPS" : @"FPS limit"];
    if (gFastReset) [a addObject:@"Fast reset"];
    if (gInfJump) [a addObject:@"Infinite jumps"];
    if (gNoBound) [a addObject:@"No boundary impact"];
    if (gAutoJump) [a addObject:@"Auto jump"];
    if (gNoclip) [a addObject:@"Noclip"];
    if (gSpin) [a addObject:@"Spin"];
    if (gScaleOn) [a addObject:@"Scale"];
    if (gTrainOn) [a addObject:@"Train speed"];
    if (gTrSpin) [a addObject:@"Spin trains"];
    if (gTrScale) [a addObject:@"Scale trains"];
    if (gAudOn) [a addObject:gAudSync ? @"Sound follows time" : [NSString stringWithFormat:@"Sound pitch %.2f", gAudPitch]];
    if (gPickOn) [a addObject:@"Pickup rotation"];
    if (gPkScale) [a addObject:@"Scale pickups"];
    if (gHideCoins) [a addObject:@"Hide coins"];
    if (gFog) [a addObject:@"Fog"];
    if (gCamOn) [a addObject:@"Camera values"];
    if (gFirstPerson) [a addObject:@"First person"];
    if (gFreecam) [a addObject:@"Freecam"];
    if (gEspOn) [a addObject:@"ESP"];
    if (gChamOn) [a addObject:@"Chams"];
    if (gTurbo) [a addObject:[NSString stringWithFormat:@"Turbo %d/s", gTurboRate]];
    if (gMacRec) [a addObject:@"Macro record"];
    if (gMacPlay) [a addObject:@"Macro play"];
    for (int i = 0; i < F_COUNT; i++) if (fl[i] && names[i]) [a addObject:[NSString stringWithUTF8String:names[i]]];
    NSMutableString *s = [NSMutableString string];
    for (NSString *x in a) [s appendFormat:@"\n+ %@", x];
    return a.count ? [NSString stringWithFormat:@"\n-- ACTIVE (%d) --%@", (int)a.count, s] : @"";
}

- (void)hudUpdate {
    NSString *pos = @"No run";
    float elapsed = 0;
    void *mo = cachedMotor();
    if (mo) {
        float p[3];
        if (motorPos(mo, p)) {
            pos = [NSString stringWithFormat:@"X %.1f Y %.1f Z %.1f", p[0], p[1], p[2]];
            void *et = mGetElapsed ? callM(mGetElapsed, mo, NULL) : NULL;
            if (et) elapsed = floatOf(et);
        } else {
            dropMotor();
        }
    }
    NSString *seed = gLastSeed ? [NSString stringWithFormat:@"%d", gLastSeed] : (gLock ? [NSString stringWithFormat:@"%d", gSeed] : @"-");
    NSString *text = [NSString stringWithFormat:@"FPS %.0f | RUN %.2fs\n%@\nSEED %@%@", _fps, elapsed, pos, seed, gHudList ? [self activeList] : @""];
    _hud.text = text;
    CGSize s = [_hud sizeThatFits:CGSizeMake(208, 2000)];
    _hud.frame = CGRectMake(8, 6, 208, s.height);
    CGRect f = _hudBox.frame;
    f.size.height = s.height + 12;
    _hudBox.frame = f;
}

- (void)tick10 {
    if (gTS) applyTimeScale();
    motorTick();
    if (gFog) fogApply(YES);
    static int skip = 0;
    if ((skip++ & 1) == 0) hideCoinsTick();
    fastResetTick();
    if (_hudOn) [self hudUpdate];
}

- (void)tick2 {
    if (gLock) {
        int n = applySeed(YES);
        if (n == 0) [self st:@"seed" set:@"Seed: waiting for RouteConfig (start a run)"];
        else if (n > 0) [self st:@"seed" set:[NSString stringWithFormat:@"Seed: LOCKED %d", gSeed]];
    }
    if (gHbMode != 0) applyHbInput();
    if (gFirstPerson) fpHide();
    worldTick();
    if (gFpsOn) fpsApply();
    if (gTrSpin || gTrScale || gPkScale) {
        void *mo = cachedMotor();
        float p[3];
        if (mo && motorPos(mo, p)) {
            if (gTrSpin || gTrScale) tlRefresh(&gTrains, "MovingTrain", p[2], -80, 900);
            if (gPkScale) tlRefresh(&gPicks, "Pickup", p[2], -30, 500);
        }
    }
    if (gMacRec || gMacPlay) [self st:@"mac" set:gMacPlay ? [NSString stringWithFormat:@"Playing %d / %d", gMacIdx, gMacN] : [NSString stringWithFormat:@"Recording: %d events", gMacN]];
    if (gCamRot || gCamPosOn || gFirstPerson || gFreecam || gCamBHop || gCamSpin[0] || gCamSpin[1] || gCamSpin[2])
        [self st:@"cam" set:[NSString stringWithFormat:@"Camera hook: %s | calls %d", hookInstalled(F_CAM) ? "installed" : "NOT installed", gCamHookCalls]];
    if (gEspOn) {
        espRefresh();
        [self st:@"esp" set:[NSString stringWithFormat:@"ESP: %d tracked, %d on screen", gEspN, gEspDrawn]];
    }
    if (gChamOn) {
        chamRefresh();
        [self st:@"cham" set:gChamMsg];
    }
}

- (void)frame:(CADisplayLink *)l {
    CFTimeInterval now = CACurrentMediaTime();
    _frame++;
    _fpsFrames++;
    if (_fpsLast == 0) _fpsLast = now;
    if (now - _fpsLast >= 0.5) {
        _fps = _fpsFrames / (float)(now - _fpsLast);
        _fpsFrames = 0;
        _fpsLast = now;
    }
    if (!il2cppReady()) return;
    float dt = (float)(l.targetTimestamp - l.timestamp);
    modelFrame(dt);
    macroFrame();
    turboFrame(dt);
    if (gAudOn || gAudVolOn) {
        static int audSkip = 0;
        if ((audSkip++ & 1) == 0) audioTick();
    }
    if (gCamOn) camTick();
    if ((gCamBendOn || gCamAutoBend) && !hookInstalled(F_CAM)) camBendTick();
    if (gTrSpin && gTrains.n) {
        float v[3] = { gTrRate[0] * dt, gTrRate[1] * dt, gTrRate[2] * dt };
        tlApply(&gTrains, mRotate, v);
    }
    if (gTrScale && gTrains.n) tlApply(&gTrains, mSetScale, gTrScaleV);
    if (gPkScale && gPicks.n) tlApply(&gPicks, mSetScale, gPkScaleV);
    if (gEspOn && !hookInstalled(F_CAM) && (_frame & 1)) {
        FZDot d[ESP_MAX];
        int n = espProject(d);
        gEspDrawn = n;
        [_esp setDots:d count:n];
    }
    if (now - _t10 >= 0.1) { _t10 = now; [self tick10]; }
    if (now - _t2 >= 0.5) { _t2 = now; [self tick2]; }
}

- (void)screenTap:(UITapGestureRecognizer *)g {
    if (gHbMode != 2) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - _lastTap > (gHbTaps <= 3 ? 0.45 : 0.6)) _tapCount = 0;
    _lastTap = now;
    if (++_tapCount < gHbTaps) return;
    _tapCount = 0;
    NSString *an = tapActions()[MAX(0, MIN((int)tapActions().count - 1, gTapAction))];
    [self st:@"hb" set:[an stringByAppendingString:tapActionRun(gTapAction) ? @": done" : @": not available"]];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self updateHb];
    });
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)g shouldReceiveTouch:(UITouch *)touch {
    UIView *v = touch.view;
    if ([v isKindOfClass:UIButton.class] || [v isKindOfClass:FZJoystick.class]) return NO;
    return !([v isDescendantOfView:_panel] || [v isDescendantOfView:_icon]);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)g shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    return YES;
}

- (void)textFieldDidBeginEditing:(UITextField *)tf {
    UIScrollView *sv = (UIScrollView *)tf.superview;
    if ([sv isKindOfClass:UIScrollView.class])
        [sv setContentOffset:CGPointMake(0, MAX(0, tf.frame.origin.y - 70)) animated:YES];
}

- (void)textFieldDidEndEditing:(UITextField *)tf {
    [self syncLive];
    [self updateHb];
}

- (BOOL)textFieldShouldReturn:(UITextField *)tf {
    [tf resignFirstResponder];
    return YES;
}

- (void)panIcon:(UIPanGestureRecognizer *)g { dragView(_icon, g); }
- (void)panPanel:(UIPanGestureRecognizer *)g { dragView(_panel, g); }

- (void)fcDown:(UIButton *)b { gFcMask |= (int)b.tag; }
- (void)fcUp:(UIButton *)b { gFcMask &= ~(int)b.tag; }

- (void)camLook:(UIPanGestureRecognizer *)g {
    CGPoint t = [g translationInView:g.view];
    [g setTranslation:CGPointZero inView:g.view];
    gFcYaw += (float)t.x * 0.15f;
    gFcPitch = clampf(gFcPitch + (float)t.y * 0.15f, -89, 89);
}

- (void)showTapActions {
    [_panel endEditing:YES];
    UIViewController *vc = keyWindow().rootViewController;
    while (vc.presentedViewController) vc = vc.presentedViewController;
    UIAlertController *a = [UIAlertController alertControllerWithTitle:@"Tap action" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    NSArray *names = tapActions();
    for (NSUInteger i = 0; i < names.count; i++) {
        NSString *n = names[i];
        [a addAction:[UIAlertAction actionWithTitle:n style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) {
            gTapAction = (int)i;
            [NSUserDefaults.standardUserDefaults setInteger:gTapAction forKey:@"fz_tapact"];
            [self->_tapBtn setTitle:[@"Action: " stringByAppendingString:n] forState:UIControlStateNormal];
            [self updateHb];
        }]];
    }
    [a addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    a.popoverPresentationController.sourceView = _panel;
    a.popoverPresentationController.sourceRect = _panel.bounds;
    [vc presentViewController:a animated:YES completion:nil];
}

- (void)toggle {
    [_panel endEditing:YES];
    _panel.hidden = !_panel.hidden;
    UIView *sv = _icon.superview;
    [sv bringSubviewToFront:_panel];
    [sv bringSubviewToFront:_icon];
    if (!_panel.hidden) {
        NSString *c = currentCity();
        if (c.length) [self st:@"map" set:[@"Current map: " stringByAppendingString:c]];
    }
}

- (void)install {
    UIWindow *w = keyWindow();
    if (!w) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self install]; });
        return;
    }
    _pages = [NSMutableArray new];
    _fields = [NSMutableDictionary new];
    _status = [NSMutableDictionary new];
    _blocks = [NSMutableDictionary new];
    _switches = [NSMutableDictionary new];
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    if ([ud objectForKey:@"fz_seed"]) gSeed = (int)[ud integerForKey:@"fz_seed"];
    if ([ud objectForKey:@"fz_tapact"]) gTapAction = (int)[ud integerForKey:@"fz_tapact"];
    if ([ud objectForKey:@"fz_taps"]) gHbTaps = (int)clampf((float)[ud integerForKey:@"fz_taps"], 2, 20);

    _esp = [[FZOverlay alloc] initWithFrame:w.bounds];
    _esp.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _esp.hidden = YES;
    [w addSubview:_esp];

    CGFloat S = 56;
    _icon = [UIButton buttonWithType:UIButtonTypeCustom];
    _icon.frame = CGRectMake(12, 120, S, S);
    _icon.layer.cornerRadius = S / 2;
    _icon.clipsToBounds = YES;
    _icon.backgroundColor = UC(0x1B1B24, 0.9);
    _icon.layer.borderWidth = 2;
    _icon.layer.borderColor = UC(0xFFFFFF, 0.3).CGColor;
#ifdef FZ_HAS_ICON
    UIImage *img = [UIImage imageWithData:[NSData dataWithBytes:icon_png length:icon_png_len]];
    if (img) {
        [_icon setImage:img forState:UIControlStateNormal];
        _icon.imageView.contentMode = UIViewContentModeScaleAspectFill;
    }
#endif
    if (![_icon imageForState:UIControlStateNormal]) {
        [_icon setTitle:@"FZ" forState:UIControlStateNormal];
        _icon.titleLabel.font = [UIFont systemFontOfSize:18 weight:UIFontWeightBold];
    }
    [_icon addTarget:self action:@selector(toggle) forControlEvents:UIControlEventTouchUpInside];
    [_icon addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panIcon:)]];
    [w addSubview:_icon];

    CGFloat pw = MIN(w.bounds.size.width - 24, 340), ph = MIN(640, w.bounds.size.height - 110);
    _pw = pw;
    _panel = [[UIView alloc] initWithFrame:CGRectMake((w.bounds.size.width - pw) / 2, 60, pw, ph)];
    _panel.backgroundColor = UC(0x15151D, 0.97);
    _panel.layer.cornerRadius = 18;
    _panel.hidden = YES;
    _panel.layer.borderWidth = 1;
    _panel.layer.borderColor = UC(0xFFFFFF, 0.4).CGColor;
    _panel.layer.shadowColor = UIColor.blackColor.CGColor;
    _panel.layer.shadowOpacity = 0.5;
    _panel.layer.shadowRadius = 12;
    _panel.layer.shadowOffset = CGSizeMake(0, 4);

    UIView *header = [[UIView alloc] initWithFrame:CGRectMake(0, 0, pw, 56)];
    header.backgroundColor = UC(0x15151D, 1);
    header.layer.cornerRadius = 18;
    header.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;
    [header addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panPanel:)]];
    [header addSubview:label(@"Training Menu", CGRectMake(16, 8, pw - 70, 24), 18, UIFontWeightBold, UIColor.whiteColor)];
    [header addSubview:label(@"for training by Frezon", CGRectMake(16, 32, pw - 70, 16), 12, UIFontWeightMedium, UC(0x7FD6C2, 1))];
    UIButton *x = [UIButton buttonWithType:UIButtonTypeCustom];
    x.frame = CGRectMake(pw - 46, 12, 32, 32);
    x.backgroundColor = UC(0x2A2A35, 1);
    x.layer.cornerRadius = 16;
    [x setTitle:@"X" forState:UIControlStateNormal];
    [x setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [header addSubview:x];
    [self bind:x event:UIControlEventTouchUpInside block:^(id s) { [self toggle]; }];
    [_panel addSubview:header];

    _tabs = [[UISegmentedControl alloc] initWithItems:@[@"Run", @"Player", @"World", @"Visual", @"Mods"]];
    _tabs.frame = CGRectMake(12, 58, pw - 24, 30);
    _tabs.selectedSegmentIndex = 0;
    _tabs.backgroundColor = UC(0x2A2A35, 1);
    _tabs.selectedSegmentTintColor = UC(0x3B7DDD, 1);
    [_tabs setTitleTextAttributes:@{ NSForegroundColorAttributeName: UIColor.whiteColor } forState:UIControlStateNormal];
    [_tabs setTitleTextAttributes:@{ NSForegroundColorAttributeName: UIColor.blackColor } forState:UIControlStateSelected];
    [_panel addSubview:_tabs];
    [self bind:_tabs event:UIControlEventValueChanged block:^(id s) {
        NSInteger i = ((UISegmentedControl *)s).selectedSegmentIndex;
        [self->_panel endEditing:YES];
        for (NSUInteger k = 0; k < self->_pages.count; k++) self->_pages[k].hidden = (k != (NSUInteger)i);
    }];

    [self buildRun];
    [self buildPlayer];
    [self buildWorld];
    [self buildVisual];
    [self buildMods];
    CGFloat WW = w.bounds.size.width, HH = w.bounds.size.height;
    _joy = [[FZJoystick alloc] initWithFrame:CGRectMake(20, HH - 190, 130, 130)];
    _joy.hidden = YES;
    [w addSubview:_joy];
    NSMutableArray *fb = [NSMutableArray array];
    for (int i = 0; i < 2; i++) {
        UIButton *b = [UIButton buttonWithType:UIButtonTypeCustom];
        b.frame = CGRectMake(WW - 84, HH - 200 + i * 76, 64, 64);
        b.backgroundColor = UC(0x000000, 0.4);
        b.layer.cornerRadius = 32;
        b.layer.borderWidth = 2;
        b.layer.borderColor = UC(0xFFFFFF, 0.6).CGColor;
        [b setTitle:i == 0 ? @"UP" : @"DN" forState:UIControlStateNormal];
        [b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        b.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
        b.tag = i == 0 ? 1 : 2;
        b.hidden = YES;
        [b addTarget:self action:@selector(fcDown:) forControlEvents:UIControlEventTouchDown];
        [b addTarget:self action:@selector(fcUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel];
        [w addSubview:b];
        [fb addObject:b];
    }
    _fcBtns = fb;
    _camPan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(camLook:)];
    _camPan.cancelsTouchesInView = NO;
    _camPan.delegate = self;
    _camPan.enabled = NO;
    [w addGestureRecognizer:_camPan];
    gEspView = _esp;
    [w addSubview:_panel];

    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(screenTap:)];
    tap.cancelsTouchesInView = NO;
    tap.delaysTouchesBegan = NO;
    tap.delaysTouchesEnded = NO;
    tap.delegate = self;
    [w addGestureRecognizer:tap];

    _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(frame:)];
    [_link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}
@end

__attribute__((constructor)) static void fz_init(void) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 8 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        [[FZTrainingMenu shared] install];
    });
}
