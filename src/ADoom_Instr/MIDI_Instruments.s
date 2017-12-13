
; MIDI Instrument File v1.0
; Joe and Mike Fenton, 10/11/95

		MC68000
		ASEG
		OBJFILE	"DEVPAC:DAMNED/MIDI_Instruments"

		INCDIR	"DEVPAC:DAMNED/raw"

;--------------------------------------------------------------------

INSTR		MACRO
		dl	11025
		dw	\1,0
		dw	\2,0
		dw	\3
		INCBIN	"\4"
		CNOP	0,4
		ENDM

;--------------------------------------------------------------------

InstrBase:
; 1-8
		dc.l	acousticgrand-InstrBase
		dc.l	brightacoustic-InstrBase
		dc.l	electricgrand-InstrBase
		dc.l	honkytonk-InstrBase
		dc.l	electricpiano1-InstrBase
		dc.l	electricpiano2-InstrBase
		dc.l	harpsichord-InstrBase
		dc.l	clavichord-InstrBase
; 9-16
		dc.l	celesta-InstrBase
		dc.l	glockenspiel-InstrBase
		dc.l	musicbox-InstrBase
		dc.l	vibraphone-InstrBase
		dc.l	marimba-InstrBase
		dc.l	xylophone-InstrBase
		dc.l	tubularbells-InstrBase
		dc.l	dulcimer-InstrBase
; 17-24
		dc.l	drawbarorgan-InstrBase
		dc.l	percussiveorgan-InstrBase
		dc.l	rockorgan-InstrBase
		dc.l	churchorgan-InstrBase
		dc.l	reedorgan-InstrBase
		dc.l	accordian-InstrBase
		dc.l	harmonica-InstrBase
		dc.l	tangoaccordian-InstrBase
; 25-32
		dc.l	acousticguitar_nylon-InstrBase
		dc.l	acousticguitar_steel-InstrBase
		dc.l	electricguitar_jazz-InstrBase
		dc.l	electricguitar_clean-InstrBase
		dc.l	electricguitar_muted-InstrBase
		dc.l	overdrivenguitar-InstrBase
		dc.l	distortionguitar-InstrBase
		dc.l	guitarharmonics-InstrBase
; 33-40
		dc.l	acousticbass-InstrBase
		dc.l	electricbass_finger-InstrBase
		dc.l	electricbass_pick-InstrBase
		dc.l	fretlessbass-InstrBase
		dc.l	slapbass1-InstrBase
		dc.l	slapbass2-InstrBase
		dc.l	synthbass1-InstrBase
		dc.l	synthbass2-InstrBase
; 41-48
		dc.l	violin-InstrBase
		dc.l	viola-InstrBase
		dc.l	cello-InstrBase
		dc.l	contrabass-InstrBase
		dc.l	tremolostrings-InstrBase
		dc.l	pizzicatostrings-InstrBase
		dc.l	orchestralstrings-InstrBase
		dc.l	timpani-InstrBase
; 49-56
		dc.l	stringensemble1-InstrBase
		dc.l	stringensemble2-InstrBase
		dc.l	synthstrings1-InstrBase
		dc.l	synthstrings2-InstrBase
		dc.l	choiraahs-InstrBase
		dc.l	voiceoohs-InstrBase
		dc.l	synthvoice-InstrBase
		dc.l	orchestrahit-InstrBase
; 57-64
		dc.l	trumpet-InstrBase
		dc.l	trombone-InstrBase
		dc.l	tuba-InstrBase
		dc.l	mutedtrumpet-InstrBase
		dc.l	frenchhorn-InstrBase
		dc.l	brasssection-InstrBase
		dc.l	synthbrass1-InstrBase
		dc.l	synthbrass2-InstrBase
; 65-72
		dc.l	sopranosax-InstrBase
		dc.l	altosax-InstrBase
		dc.l	tenorsax-InstrBase
		dc.l	baritonesax-InstrBase
		dc.l	oboe-InstrBase
		dc.l	englishhorn-InstrBase
		dc.l	bassoon-InstrBase
		dc.l	clarinet-InstrBase
; 73-80
		dc.l	piccolo-InstrBase
		dc.l	flute-InstrBase
		dc.l	recorder-InstrBase
		dc.l	panflute-InstrBase
		dc.l	blownbottle-InstrBase
		dc.l	shakuhachi-InstrBase
		dc.l	whistle-InstrBase
		dc.l	ocarina-InstrBase
; 81-88
		dc.l	lead1_square-InstrBase
		dc.l	lead2_sawtooth-InstrBase
		dc.l	lead3_calliope-InstrBase
		dc.l	lead4_chiff-InstrBase
		dc.l	lead5_charang-InstrBase
		dc.l	lead6_voice-InstrBase
		dc.l	lead7_fifths-InstrBase
		dc.l	lead8_bass-InstrBase
; 89-96
		dc.l	pad1_newage-InstrBase
		dc.l	pad2_warm-InstrBase
		dc.l	pad3_polysynth-InstrBase
		dc.l	pad4_choir-InstrBase
		dc.l	pad5_bowed-InstrBase
		dc.l	pad6_metallic-InstrBase
		dc.l	pad7_halo-InstrBase
		dc.l	pad8_sweep-InstrBase
; 97-104
		dc.l	fx1_rain-InstrBase
		dc.l	fx2_soundtrack-InstrBase
		dc.l	fx3_crystal-InstrBase
		dc.l	fx4_atmosphere-InstrBase
		dc.l	fx5_brightness-InstrBase
		dc.l	fx6_goblins-InstrBase
		dc.l	fx7_echoes-InstrBase
		dc.l	fx8_scifi-InstrBase
; 105-112
		dc.l	sitar-InstrBase
		dc.l	banjo-InstrBase
		dc.l	shamisen-InstrBase
		dc.l	koto-InstrBase
		dc.l	kalimba-InstrBase
		dc.l	bagpipe-InstrBase
		dc.l	fiddle-InstrBase
		dc.l	shanai-InstrBase
; 113-120
		dc.l	tinklebell-InstrBase
		dc.l	agogo-InstrBase
		dc.l	steeldrums-InstrBase
		dc.l	woodblock-InstrBase
		dc.l	taikodrum-InstrBase
		dc.l	melodictom-InstrBase
		dc.l	synthdrum-InstrBase
		dc.l	reversecymbal-InstrBase
; 121-128
		dc.l	guitarfretnoise-InstrBase
		dc.l	breathnoise-InstrBase
		dc.l	seashore-InstrBase
		dc.l	birdtweet-InstrBase
		dc.l	telephonering-InstrBase
		dc.l	helicopter-InstrBase
		dc.l	applause-InstrBase
		dc.l	gunshot-InstrBase


		dc.l	0,0,0,0,0,0,0

; 135-181
		dc.l	bassdrum1-InstrBase	; acoustic bass drum
		dc.l	bassdrum1-InstrBase
		dc.l	sidestick-InstrBase
		dc.l	acousticsnare-InstrBase
		dc.l	handclap-InstrBase
		dc.l	electricsnare-InstrBase
		dc.l	lowfloortom-InstrBase
		dc.l	closedhihat-InstrBase
		dc.l	highfloortom-InstrBase
		dc.l	pedalhihat-InstrBase
		dc.l	lowtom-InstrBase
		dc.l	openhihat-InstrBase
		dc.l	lowmidtom-InstrBase
		dc.l	highmidtom-InstrBase
		dc.l	crashcymbal1-InstrBase
		dc.l	hightom-InstrBase
		dc.l	ridecymbal1-InstrBase
		dc.l	chinesecymbal-InstrBase
		dc.l	ridebell-InstrBase
		dc.l	tambourine-InstrBase
		dc.l	splashcymbal-InstrBase
		dc.l	cowbell-InstrBase
		dc.l	crashcymbal2-InstrBase
		dc.l	vibraslap-InstrBase
		dc.l	ridecymbal2-InstrBase
		dc.l	highbongo-InstrBase
		dc.l	lowbongo-InstrBase
		dc.l	mutehiconga-InstrBase
		dc.l	openhiconga-InstrBase
		dc.l	lowconga-InstrBase
		dc.l	hightimbale-InstrBase
		dc.l	lowtimbale-InstrBase
		dc.l	highagogo-InstrBase
		dc.l	lowagogo-InstrBase
		dc.l	cabasa-InstrBase
		dc.l	maracas-InstrBase
		dc.l	shortwhistle-InstrBase
		dc.l	longwhistle-InstrBase
		dc.l	shortguiro-InstrBase
		dc.l	longguiro-InstrBase
		dc.l	claves-InstrBase
		dc.l	highwoodblock-InstrBase
		dc.l	lowwoodblock-InstrBase
		dc.l	mutecuica-InstrBase
		dc.l	opencuica-InstrBase
		dc.l	mutetriangle-InstrBase
		dc.l	opentriangle-InstrBase

;--------------------------------------------------------------------

		CNOP	0,16

acousticgrand	INSTR	0,36626,60,mi_0
brightacoustic	INSTR	0,34988,60,mi_1
electricgrand	INSTR	0,48006,60,mi_2
honkytonk	INSTR	0,42180,60,mi_3
electricpiano1	INSTR	0,61456,60,mi_4
electricpiano2	INSTR	0,35658,60,mi_5
harpsichord	INSTR	0,15078,60,mi_6
clavichord	INSTR	0,52092,60,mi_7

celesta		INSTR	0,31050,60,mi_8
glockenspiel	INSTR	0,11052,60,mi_9
musicbox	INSTR	0,42022,60,mi_10
vibraphone	INSTR	0,32668,60,mi_11
marimba		INSTR	0,4164,60,mi_12
xylophone	INSTR	0,4350,60,mi_13
tubularbells	INSTR	0,21074,48,mi_14
dulcimer	INSTR	0,29020,60,mi_15

drawbarorgan	INSTR	5466,16714,60,mi_16
percussiveorgan	INSTR	4128,19448,60,mi_17
rockorgan	INSTR	10156,35104,60,mi_18
churchorgan	INSTR	14084,16012,60,mi_19
reedorgan	INSTR	1230,3846,60,mi_20
accordian	INSTR	3916,6068,60,mi_21
harmonica	INSTR	20146,24190,60,mi_22
tangoaccordian	INSTR	9076,16502,60,mi_23

acousticguitar_nylon	INSTR	0,33404,60,mi_24
acousticguitar_steel	INSTR	0,29864,60,mi_25
electricguitar_jazz	INSTR	0,38132,60,mi_26
electricguitar_clean	INSTR	0,28648,60,mi_27
electricguitar_muted	INSTR	0,24914,60,mi_28
overdrivenguitar	INSTR	18054,25228,60,mi_29
distortionguitar	INSTR	12054,18172,60,mi_30
guitarharmonics		INSTR	0,34375,60,mi_31

acousticbass		INSTR	0,33484,36,mi_32
electricbass_finger	INSTR	0,37376,36,mi_33
electricbass_pick	INSTR	0,37072,36,mi_34
fretlessbass	INSTR	0,38378,36,mi_35
slapbass1	INSTR	0,39708,36,mi_36
slapbass2	INSTR	0,35118,36,mi_37
synthbass1	INSTR	3378,7936,36,mi_38
synthbass2	INSTR	1690,5404,36,mi_39

violin		INSTR	7064,10102,60,mi_40
viola		INSTR	12680,16646,60,mi_41
cello		INSTR	3946,7576,48,mi_42
contrabass	INSTR	6036,7386,36,mi_43
tremolostrings	INSTR	3570,16950,60,mi_44
pizzicatostrings	INSTR	0,2204,60,mi_45
orchestralstrings	INSTR	0,17982,48,mi_46
timpani			INSTR	0,6334,60,mi_47

stringensemble1	INSTR	7178,20016,60,mi_48
stringensemble2	INSTR	10444,16882,60,mi_49
synthstrings1	INSTR	4474,21134,60,mi_50
synthstrings2	INSTR	4886,12832,60,mi_51
choiraahs	INSTR	22892,44436,48,mi_52
voiceoohs	INSTR	34436,49170,48,mi_53
synthvoice	INSTR	9494,26626,48,mi_54
orchestrahit	INSTR	0,26244,48,mi_55

trumpet		INSTR	2674,5332,60,mi_56
trombone	INSTR	11662,15380,48,mi_57
tuba		INSTR	13128,14474,36,mi_58
mutedtrumpet	INSTR	9098,9478,60,mi_59
frenchhorn	INSTR	13588,15446,48,mi_60
brasssection	INSTR	5364,6756,60,mi_61
synthbrass1	INSTR	8796,11220,60,mi_62
synthbrass2	INSTR	16406,16852,60,mi_63

sopranosax	INSTR	2146,7590,60,mi_64
altosax		INSTR	3160,8012,60,mi_65
tenorsax	INSTR	3890,9756,60,mi_66
baritonesax	INSTR	3514,9170,48,mi_67
oboe		INSTR	4734,12582,60,mi_68
englishhorn	INSTR	5824,13336,48,mi_69
bassoon		INSTR	816,1320,36,mi_70
clarinet	INSTR	1176,2062,60,mi_71

piccolo		INSTR	2244,2898,72,mi_72
flute		INSTR	3416,3754,60,mi_73
recorder	INSTR	3836,4216,60,mi_74
panflute	INSTR	1574,2460,60,mi_75
blownbottle	INSTR	3132,5750,60,mi_76
shakuhachi	INSTR	2586,3342,60,mi_77
whistle		INSTR	1454,1834,60,mi_78
ocarina		INSTR	1698,2416,60,mi_79

lead1_square	INSTR	4684,11332,60,mi_80
lead2_sawtooth	INSTR	5308,12164,60,mi_81
lead3_calliope	INSTR	3028,10832,60,mi_82
lead4_chiff	INSTR	8756,12554,60,mi_83
lead5_charang	INSTR	21928,24210,60,mi_84
lead6_voice	INSTR	3960,11094,60,mi_85
lead7_fifths	INSTR	1002,1846,60,mi_86
lead8_bass	INSTR	6072,12422,60,mi_87

pad1_newage	INSTR	5486,6162,60,mi_88
pad2_warm	INSTR	14828,20728,60,mi_89
pad3_polysynth	INSTR	5012,12966,60,mi_90
pad4_choir	INSTR	3018,10440,60,mi_91
pad5_bowed	INSTR	25492,29802,60,mi_92
pad6_metallic	INSTR	14978,22880,60,mi_93
pad7_halo	INSTR	15916,23708,60,mi_94
pad8_sweep	INSTR	27298,30820,60,mi_95

fx1_rain	INSTR	10030,23958,48,mi_96
fx2_soundtrack	INSTR	19018,20628,60,mi_97
fx3_crystal	INSTR	0,30384,60,mi_98
fx4_atmosphere	INSTR	12954,20472,60,mi_99
fx5_brightness	INSTR	28656,31988,60,mi_100
fx6_goblins	INSTR	24842,39316,60,mi_101
fx7_echoes	INSTR	9846,18042,60,mi_102
fx8_scifi	INSTR	10807,17822,48,mi_103

sitar		INSTR	0,21190,60,mi_104
banjo		INSTR	0,15182,60,mi_105
shamisen	INSTR	0,11204,60,mi_106
koto		INSTR	0,24114,60,mi_107
kalimba		INSTR	0,9746,60,mi_108
bagpipe		INSTR	5362,7514,60,mi_109
fiddle		INSTR	6561,10610,60,mi_110
shanai		INSTR	1568,4268,60,mi_111

tinklebell	INSTR	0,12306,48,mi_112
agogo		INSTR	0,10362,60,mi_113
steeldrums	INSTR	0,7726,60,mi_114
woodblock	INSTR	0,1164,60,mi_115
taikodrum	INSTR	0,6158,36,mi_116
melodictom	INSTR	0,7686,48,mi_117
synthdrum	INSTR	0,4328,48,mi_118
reversecymbal	INSTR	0,12080,48,mi_119

guitarfretnoise	INSTR	0,1632,60,mi_120
breathnoise	INSTR	0,1498,60,mi_121
seashore	INSTR	2,43742,60,mi_122
birdtweet	INSTR	462,3194,60,mi_123
telephonering	INSTR	2,2242,60,mi_124
helicopter	INSTR	10398,13756,60,mi_125
applause	INSTR	10492,21368,60,mi_126
gunshot		INSTR	0,4220,60,mi_127


; acoustic bass drum
bassdrum1	INSTR	0,2182,60,mp_1
sidestick	INSTR	0,1070,60,mp_2
acousticsnare	INSTR	0,3414,60,mp_3
handclap	INSTR	0,1800,60,mp_4
electricsnare	INSTR	0,4790,60,mp_5
lowfloortom	INSTR	0,7946,60,mp_6
closedhihat	INSTR	0,1048,60,mp_7
highfloortom	INSTR	0,5524,60,mp_8
pedalhihat	INSTR	0,1242,60,mp_9
lowtom		INSTR	0,4680,60,mp_10
openhihat	INSTR	0,3996,60,mp_11
lowmidtom	INSTR	0,5622,60,mp_12
highmidtom	INSTR	0,4466,60,mp_13
crashcymbal1	INSTR	0,17822,60,mp_14
hightom		INSTR	0,3754,60,mp_15
ridecymbal1	INSTR	0,12564,60,mp_16
chinesecymbal	INSTR	0,19908,60,mp_17
ridebell	INSTR	0,10646,60,mp_18
tambourine	INSTR	0,1458,60,mp_19
splashcymbal	INSTR	0,6320,60,mp_20
cowbell		INSTR	0,1078,60,mp_21
crashcymbal2	INSTR	0,19486,60,mp_22
vibraslap	INSTR	0,8274,60,mp_23
ridecymbal2	INSTR	0,10026,60,mp_24
highbongo	INSTR	0,480,60,mp_25
lowbongo	INSTR	0,756,60,mp_26
mutehiconga	INSTR	0,1390,60,mp_27
openhiconga	INSTR	0,2852,60,mp_28
lowconga	INSTR	0,3762,60,mp_29
hightimbale	INSTR	0,2392,60,mp_30
lowtimbale	INSTR	0,2872,60,mp_31
highagogo	INSTR	0,2630,60,mp_32
lowagogo	INSTR	0,3916,60,mp_33
cabasa		INSTR	0,762,60,mp_34
maracas		INSTR	0,672,60,mp_35
shortwhistle	INSTR	0,1748,60,mp_36
longwhistle	INSTR	0,5814,60,mp_37
shortguiro	INSTR	0,736,60,mp_38
longguiro	INSTR	0,1998,60,mp_39
claves		INSTR	0,1258,60,mp_40
highwoodblock	INSTR	0,720,60,mp_41
lowwoodblock	INSTR	0,1188,60,mp_42
mutecuica	INSTR	0,1120,60,mp_43
opencuica	INSTR	0,2060,60,mp_44
mutetriangle	INSTR	0,854,60,mp_45
opentriangle	INSTR	0,9252,60,mp_46

;--------------------------------------------------------------------


		END

