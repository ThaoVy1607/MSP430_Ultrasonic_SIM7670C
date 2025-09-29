// Default Configuration definitions from GUI

/*! Macro to calculate number of DelayTimer counts according to desired ms */

//#define XT2_FREQ            (8000000)           /*! XT2 Frequency */
//
///*! MCLK=SMCLK Frequency used by default, except for upstream/downstream sampling */
//#define MCLK_FREQ             (8000000)   // Running from DCO
///*! MCLK Frequency used during upstream/downstream sampling */
//#define SAMPLING_MCLK_FREQ    (4000000)   // Running from HFXT/2

/////////////////////////////////////////////////////////////////////////
// USS
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// FROM GUI
/////////////////////////////////////////////////////////////////////////

//#define ADC_SAMPLING_FREQ_MHZ_DEFAULT           (uint16_t) (72)    /*! ADC sampling frequency in MHz */ // todo: remove
//#define ADC_OVERSAMPLE_RATE_DEFAULT             (20)    /* SIG_SAMPLING_FREQ = ADC_SAMPLING_RATE / ADC_OVERSAMP_RATE */ // todo: remove
//#define CAPTURE_DURATION_DEFAULT                (40e-6) // todo: remove

//#define UPS_DNS_LENGTH_DEFAULT                  DELAYTIMER_MS(8) //(uint16_t)(SAMPLING_MCLK_FREQ*8e-3)  /*! MCLK cycles for a complete loop */
//#define METER_CONST_DEFAULT                     (112.6)
//#define PULSE_STRT_GAP_ADC_CAPTURE_DEFAULT      (40e-6)
//#define KHZ_2_HZ_SCALE                          (1000)  /* Used when KHZ to HZ conversion is required */
//#define DELTA_OFFSET_PSEC_DEFAULT               (0)
//#define CAPTURE_DURATION_DEFAULT                (40e-6)

// Enumerated values that are supported
//#define ADC_SAMPLING_FREQ_68_MHZ               (68)    /*! ADC sampling frequency */
//#define ADC_SAMPLING_FREQ_72_MHZ               (72)
//#define ADC_SAMPLING_FREQ_76_MHZ               (76)    /*! ADC sampling frequency */
//#define ADC_SAMPLING_FREQ_80_MHZ               (80)
//#define SIG_SAMPLING_FREQ_3400_KHZ             (3400)   /*! Effective signal sampling frequency */
//#define SIG_SAMPLING_FREQ_3600_KHZ             (3600)
//#define SIG_SAMPLING_FREQ_3800_KHZ             (3800)
//#define SIG_SAMPLING_FREQ_4000_KHZ             (4000)

// DEVELOPER CONFIGURATION GUI WINDOW
// PARAM4, PARAM7-10 are defined for specific purposes
#define ENV_CROSS_THRESHOLD_TIMES_PERC  (50)
#define NEG_RNG_DLT_US     (10)
#define POS_RNG_DLT_US     (10)
#define NEG_RNG_ABS_US     (30)
#define POS_RNG_ABS_US     (8)
//
//// This showed go away once the GUI is updated as this will be part of the Library
//#define TX_BIN_ARRAY_SIZE_MAX       (2000)

// Derived DEFAULT parameters
//#define DMA_START_DELAY_DEFAULT                 (uint16_t)(SAMPLING_MCLK_FREQ*PULSE_STRT_GAP_ADC_CAPTURE_DEFAULT)  /*! MCLK Cycles to wait before starting ADC Sampling using DMA */
//#define ADC_SAMPLING_FREQ_KHZ_DEFAULT           (uint32_t) (ADC_SAMPLING_FREQ_MHZ_DEFAULT*1000)    /*! ADC sampling frequency in KHz */
//#define ADC_SAMPLING_FREQ_DEFAULT               (uint32_t) (ADC_SAMPLING_FREQ_KHZ_DEFAULT*1000)    /*! ADC sampling frequency in KHz */
//#define SIG_SAMPLING_FREQ_KHZ_DEFAULT           (uint16_t)(ADC_SAMPLING_FREQ_KHZ_DEFAULT/ADC_OVERSAMPLE_RATE_DEFAULT) // (3600)   /*! Effective signal sampling frequency */
//#define SIG_SAMPLING_FREQ_DEFAULT               (uint32_t)(SIG_SAMPLING_FREQ_KHZ_DEFAULT*1000) // (3600)   /*! Effective signal sampling frequency */
//#define ARRAY_SIZE		(uint16_t)(CAPTURE_DURATION_DEFAULT*SIG_SAMPLING_FREQ_KHZ_DEFAULT*1000)
//#define TX_BIN_ARRAY_SIZE           (1000)
//#define TX_DEC_ARRAY_SIZE           (125)
//#define UPSAMP_FACTOR           ADC_OVERSAMPLE_RATE_DEFAULT

/////////////////////////////////////////////////////////////////////////
// NOT IN GUI; SOME DEFAULT SETTINGS USED TO TEST etc.
/////////////////////////////////////////////////////////////////////////

