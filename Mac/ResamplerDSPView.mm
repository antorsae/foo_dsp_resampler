#import "stdafx.h"
#import "ResamplerDSPView.h"
#import "dsp_config.h"

@interface ResamplerDSPView ()
@property (nonatomic) dsp_preset_edit_callback_v2::ptr callback;
@property (nonatomic, strong) NSComboBox *rateCombo;
@property (nonatomic, strong) NSComboBox *qualCombo;
@property (nonatomic, strong) NSButton *aliasCheckbox;
@property (nonatomic, strong) NSSlider *passbandSlider;
@property (nonatomic, strong) NSSlider *phaseSlider;
@property (nonatomic, strong) NSTextField *passbandLabel;
@property (nonatomic, strong) NSTextField *phaseLabel;
@property (nonatomic, strong) NSComboBox *specialModeCombo;
@property (nonatomic, strong) NSTextField *specialRatesField;
@property (nonatomic) t_dsp_rate_params *params;
@end

@implementation ResamplerDSPView

static NSArray *sSampleRates;
static NSArray *sQualities;
static NSArray *sSpecialModes;

+ (void)initialize {
    sSampleRates = @[@"8000", @"11025", @"16000", @"22050", @"24000", @"32000",
                     @"44100", @"48000", @"64000", @"88200", @"93750", @"96000",
                     @"176400", @"192000",
                     @"Upsample x2", @"Upsample x4", @"Downsample x2",
                     @"Downsample x4", @"Upsample x8", @"Upsample x16",
                     @"Up to 176/192 kHz", @"Up to 352/384 kHz", @"Up to 705/768kHz"];
#if fb_sample_t_bits == 64
    sQualities = @[@"Best 53 (319 dB)", @"Ultra 47 (282 dB)", @"Ultra 37 (222 dB)", @"Ultra (168 dB)", @"High (120 dB)", @"Fast (96 dB)"];
#else
    sQualities = @[@"Ultra 28 (168 dB)", @"High (120 dB)", @"Fast (96 dB)"];
#endif
    sSpecialModes = @[@"No special sample rate handling - normal operation",
                      @"Resample ONLY these rates:",
                      @"Resample everything EXCEPT these rates:"];
}

- (instancetype)init {
    self = [super initWithNibName:nil bundle:nil];
    return self;
}

- (void)loadView {
    NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 420, 360)];

    CGFloat y = 330;
    CGFloat labelW = 130, ctrlX = 140, ctrlW = 260, rowH = 26;

    // Sample rate
    [self addLabel:@"Sample rate:" toView:view atY:y width:labelW];
    _rateCombo = [[NSComboBox alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW, 22)];
    _rateCombo.dataSource = self;
    _rateCombo.completes = NO;
    _rateCombo.delegate = (id<NSComboBoxDelegate>)self;
    [_rateCombo addItemsWithObjectValues:sSampleRates];
    [view addSubview:_rateCombo];
    y -= rowH;

    // Quality
    [self addLabel:@"Quality:" toView:view atY:y width:labelW];
    _qualCombo = [[NSComboBox alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW, 22)];
    [_qualCombo addItemsWithObjectValues:sQualities];
    [view addSubview:_qualCombo];
    y -= rowH;

    // Aliasing
    _aliasCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW, 18)];
    [_aliasCheckbox setButtonType:NSButtonTypeSwitch];
    [_aliasCheckbox setTitle:@"Allow aliasing/imaging"];
    [view addSubview:_aliasCheckbox];
    y -= rowH;

    // Passband
    [self addLabel:@"Passband:" toView:view atY:y width:labelW];
    _passbandSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW - 60, 20)];
    [_passbandSlider setMinValue:minPassband10];
    [_passbandSlider setMaxValue:maxPassband10];
    [_passbandSlider setTarget:self];
    [_passbandSlider setAction:@selector(onSlider:)];
    [view addSubview:_passbandSlider];
    _passbandLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(ctrlX + ctrlW - 55, y, 55, 20)];
    [_passbandLabel setBezeled:NO];
    [_passbandLabel setEditable:NO];
    [_passbandLabel setDrawsBackground:NO];
    [view addSubview:_passbandLabel];
    y -= rowH;

    // Phase
    [self addLabel:@"Phase response:" toView:view atY:y width:labelW];
    _phaseSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW - 60, 20)];
    [_phaseSlider setMinValue:Pminimum];
    [_phaseSlider setMaxValue:Plinear];
    [_phaseSlider setTarget:self];
    [_phaseSlider setAction:@selector(onSlider:)];
    [view addSubview:_phaseSlider];
    _phaseLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(ctrlX + ctrlW - 55, y, 55, 20)];
    [_phaseLabel setBezeled:NO];
    [_phaseLabel setEditable:NO];
    [_phaseLabel setDrawsBackground:NO];
    [view addSubview:_phaseLabel];
    y -= rowH;

    // Special mode
    [self addLabel:@"Special rates:" toView:view atY:y width:labelW];
    _specialModeCombo = [[NSComboBox alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW, 22)];
    [_specialModeCombo addItemsWithObjectValues:sSpecialModes];
    [view addSubview:_specialModeCombo];
    y -= rowH;

    // Special rates field
    [self addLabel:@"Rates (e.g. 88200-96000):" toView:view atY:y width:labelW];
    _specialRatesField = [[NSTextField alloc] initWithFrame:NSMakeRect(ctrlX, y, ctrlW, 22)];
    [view addSubview:_specialRatesField];
    y -= rowH;

    self.view = view;
}

- (void)addLabel:(NSString *)text toView:(NSView *)view atY:(CGFloat)y width:(CGFloat)w {
    NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(10, y, w, 17)];
    [label setStringValue:text];
    [label setBezeled:NO];
    [label setEditable:NO];
    [label setDrawsBackground:NO];
    [view addSubview:label];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    dsp_preset_impl preset;
    _callback->get_preset(preset);

    _params = new t_dsp_rate_params();
    _params->set_data(preset);

    char buf[30];
    _params->toutRateStr(buf);
    NSString *rateStr = [NSString stringWithUTF8String:buf];
    NSUInteger rateIdx = [sSampleRates indexOfObject:rateStr];
    if (rateIdx != NSNotFound) {
        [_rateCombo selectItemAtIndex:rateIdx];
    } else {
        [_rateCombo setStringValue:rateStr];
    }

    [_qualCombo selectItemAtIndex:(NSInteger)(_params->quality() - Qvalmin)];

    [_aliasCheckbox setState:(_params->aliasing() ? NSControlStateValueOn : NSControlStateValueOff)];
    [_passbandSlider setDoubleValue:_params->passband10()];
    [_phaseSlider setDoubleValue:_params->phase()];
    [self updatePassbandLabel];
    [self updatePhaseLabel];

    [_specialModeCombo selectItemAtIndex:_params->specialmode()];
    [_specialRatesField setStringValue:[NSString stringWithUTF8String:_params->SpecialRatesStr().get_ptr()]];
}

- (void)onSlider:(id)sender {
    if (sender == _passbandSlider) {
        [self updatePassbandLabel];
    } else if (sender == _phaseSlider) {
        [self updatePhaseLabel];
    }
    [self apply];
}

- (void)updatePassbandLabel {
    double val = [_passbandSlider doubleValue];
    [_passbandLabel setStringValue:[NSString stringWithFormat:@"%.1f%%", val / 10.0]];
}

- (void)updatePhaseLabel {
    int val = (int)[_phaseSlider doubleValue];
    if (val == Pminimum) {
        [_phaseLabel setStringValue:@"0% (min)"];
    } else if (val == Plinear) {
        [_phaseLabel setStringValue:@"50% (lin)"];
    } else if (val == Pmaximum) {
        [_phaseLabel setStringValue:@"100% (max)"];
    } else {
        [_phaseLabel setStringValue:[NSString stringWithFormat:@"%d%%", val]];
    }
}

- (void)controlTextDidChange:(NSNotification *)notification {
    [self apply];
}

- (void)apply {
    if (!_params) return;

    NSString *rateStr = [_rateCombo stringValue];
    _params->tset_outRate([rateStr UTF8String]);

    _params->set_quality((t_int32)([_qualCombo indexOfSelectedItem] >= 0 ? [_qualCombo indexOfSelectedItem] : 0) + Qvalmin);
    _params->set_aliasing([_aliasCheckbox state] == NSControlStateValueOn ? 1 : 0);
    _params->set_passband10((t_int32)[_passbandSlider doubleValue]);
    _params->set_phase((t_int32)[_phaseSlider doubleValue]);
    _params->set_specialmode((t_int32)([_specialModeCombo indexOfSelectedItem] >= 0 ? [_specialModeCombo indexOfSelectedItem] : 0));
    _params->set_specialrates([[_specialRatesField stringValue] UTF8String]);

    dsp_preset_impl outPreset;
    _params->get_data(outPreset);
    _callback->set_preset(outPreset);
}

- (void)dealloc {
    delete _params;
}

#pragma mark NSComboBoxDataSource

- (NSInteger)numberOfItemsInComboBox:(NSComboBox *)aComboBox {
    if (aComboBox == _rateCombo) return [sSampleRates count];
    return 0;
}

- (id)comboBox:(NSComboBox *)aComboBox objectValueForItemAtIndex:(NSInteger)index {
    if (aComboBox == _rateCombo) return sSampleRates[index];
    return @"";
}

@end

service_ptr ConfigureResamplerDSP(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) {
    ResamplerDSPView *dialog = [ResamplerDSPView new];
    dialog.callback = callback;
    return fb2k::wrapNSObject(dialog);
}
