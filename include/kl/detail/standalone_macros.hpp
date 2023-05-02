#pragma once

// workaround for old msvc preprocessor:
// https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define KL_MSVC_EXP(x) x

#define KL_FE_0(f)
#define KL_FE_1(f, x) f(x)
#define KL_FE_2(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_1(f, __VA_ARGS__))
#define KL_FE_3(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_2(f, __VA_ARGS__))
#define KL_FE_4(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_3(f, __VA_ARGS__))
#define KL_FE_5(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_4(f, __VA_ARGS__))
#define KL_FE_6(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_5(f, __VA_ARGS__))
#define KL_FE_7(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_6(f, __VA_ARGS__))
#define KL_FE_8(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_7(f, __VA_ARGS__))
#define KL_FE_9(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_8(f, __VA_ARGS__))

#define KL_FE_10(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_9(f, __VA_ARGS__))
#define KL_FE_11(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_10(f, __VA_ARGS__))
#define KL_FE_12(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_11(f, __VA_ARGS__))
#define KL_FE_13(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_12(f, __VA_ARGS__))
#define KL_FE_14(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_13(f, __VA_ARGS__))
#define KL_FE_15(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_14(f, __VA_ARGS__))
#define KL_FE_16(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_15(f, __VA_ARGS__))
#define KL_FE_17(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_16(f, __VA_ARGS__))
#define KL_FE_18(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_17(f, __VA_ARGS__))
#define KL_FE_19(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_18(f, __VA_ARGS__))

#define KL_FE_20(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_19(f, __VA_ARGS__))
#define KL_FE_21(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_20(f, __VA_ARGS__))
#define KL_FE_22(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_21(f, __VA_ARGS__))
#define KL_FE_23(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_22(f, __VA_ARGS__))
#define KL_FE_24(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_23(f, __VA_ARGS__))
#define KL_FE_25(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_24(f, __VA_ARGS__))
#define KL_FE_26(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_25(f, __VA_ARGS__))
#define KL_FE_27(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_26(f, __VA_ARGS__))
#define KL_FE_28(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_27(f, __VA_ARGS__))
#define KL_FE_29(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_28(f, __VA_ARGS__))

#define KL_FE_30(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_29(f, __VA_ARGS__))
#define KL_FE_31(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_30(f, __VA_ARGS__))
#define KL_FE_32(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_31(f, __VA_ARGS__))
#define KL_FE_33(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_32(f, __VA_ARGS__))
#define KL_FE_34(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_33(f, __VA_ARGS__))
#define KL_FE_35(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_34(f, __VA_ARGS__))
#define KL_FE_36(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_35(f, __VA_ARGS__))
#define KL_FE_37(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_36(f, __VA_ARGS__))
#define KL_FE_38(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_37(f, __VA_ARGS__))
#define KL_FE_39(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_38(f, __VA_ARGS__))

#define KL_FE_40(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_39(f, __VA_ARGS__))
#define KL_FE_41(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_40(f, __VA_ARGS__))
#define KL_FE_42(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_41(f, __VA_ARGS__))
#define KL_FE_43(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_42(f, __VA_ARGS__))
#define KL_FE_44(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_43(f, __VA_ARGS__))
#define KL_FE_45(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_44(f, __VA_ARGS__))
#define KL_FE_46(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_45(f, __VA_ARGS__))
#define KL_FE_47(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_46(f, __VA_ARGS__))
#define KL_FE_48(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_47(f, __VA_ARGS__))
#define KL_FE_49(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_48(f, __VA_ARGS__))

#define KL_FE_50(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_49(f, __VA_ARGS__))
#define KL_FE_51(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_50(f, __VA_ARGS__))
#define KL_FE_52(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_51(f, __VA_ARGS__))
#define KL_FE_53(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_52(f, __VA_ARGS__))
#define KL_FE_54(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_53(f, __VA_ARGS__))
#define KL_FE_55(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_54(f, __VA_ARGS__))
#define KL_FE_56(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_55(f, __VA_ARGS__))
#define KL_FE_57(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_56(f, __VA_ARGS__))
#define KL_FE_58(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_57(f, __VA_ARGS__))
#define KL_FE_59(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_58(f, __VA_ARGS__))

#define KL_FE_60(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_59(f, __VA_ARGS__))
#define KL_FE_61(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_60(f, __VA_ARGS__))
#define KL_FE_62(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_61(f, __VA_ARGS__))
#define KL_FE_63(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_62(f, __VA_ARGS__))
#define KL_FE_64(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_63(f, __VA_ARGS__))
#define KL_FE_65(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_64(f, __VA_ARGS__))
#define KL_FE_66(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_65(f, __VA_ARGS__))
#define KL_FE_67(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_66(f, __VA_ARGS__))
#define KL_FE_68(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_67(f, __VA_ARGS__))
#define KL_FE_69(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_68(f, __VA_ARGS__))

#define KL_FE_70(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_69(f, __VA_ARGS__))
#define KL_FE_71(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_70(f, __VA_ARGS__))
#define KL_FE_72(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_71(f, __VA_ARGS__))
#define KL_FE_73(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_72(f, __VA_ARGS__))
#define KL_FE_74(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_73(f, __VA_ARGS__))
#define KL_FE_75(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_74(f, __VA_ARGS__))
#define KL_FE_76(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_75(f, __VA_ARGS__))
#define KL_FE_77(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_76(f, __VA_ARGS__))
#define KL_FE_78(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_77(f, __VA_ARGS__))
#define KL_FE_79(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_78(f, __VA_ARGS__))

#define KL_FE_80(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_79(f, __VA_ARGS__))
#define KL_FE_81(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_80(f, __VA_ARGS__))
#define KL_FE_82(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_81(f, __VA_ARGS__))
#define KL_FE_83(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_82(f, __VA_ARGS__))
#define KL_FE_84(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_83(f, __VA_ARGS__))
#define KL_FE_85(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_84(f, __VA_ARGS__))
#define KL_FE_86(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_85(f, __VA_ARGS__))
#define KL_FE_87(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_86(f, __VA_ARGS__))
#define KL_FE_88(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_87(f, __VA_ARGS__))
#define KL_FE_89(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_88(f, __VA_ARGS__))

#define KL_FE_90(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_89(f, __VA_ARGS__))
#define KL_FE_91(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_90(f, __VA_ARGS__))
#define KL_FE_92(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_91(f, __VA_ARGS__))
#define KL_FE_93(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_92(f, __VA_ARGS__))
#define KL_FE_94(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_93(f, __VA_ARGS__))
#define KL_FE_95(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_94(f, __VA_ARGS__))
#define KL_FE_96(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_95(f, __VA_ARGS__))
#define KL_FE_97(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_96(f, __VA_ARGS__))
#define KL_FE_98(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_97(f, __VA_ARGS__))
#define KL_FE_99(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_98(f, __VA_ARGS__))

#define KL_FE_100(f, x, ...) f(x), KL_MSVC_EXP(KL_FE_99(f, __VA_ARGS__))

#define KL_MATCH_ARGS(                                                         \
    ign0, ign1, ign2, ign3, ign4, ign5, ign6, ign7, ign8, ign9, ign10, ign11,  \
    ign12, ign13, ign14, ign15, ign16, ign17, ign18, ign19, ign20, ign21,      \
    ign22, ign23, ign24, ign25, ign26, ign27, ign28, ign29, ign30, ign31,      \
    ign32, ign33, ign34, ign35, ign36, ign37, ign38, ign39, ign40, ign41,      \
    ign42, ign43, ign44, ign45, ign46, ign47, ign48, ign49, ign50, ign51,      \
    ign52, ign53, ign54, ign55, ign56, ign57, ign58, ign59, ign60, ign61,      \
    ign62, ign63, ign64, ign65, ign66, ign67, ign68, ign69, ign70, ign71,      \
    ign72, ign73, ign74, ign75, ign76, ign77, ign78, ign79, ign80, ign81,      \
    ign82, ign83, ign84, ign85, ign86, ign87, ign88, ign89, ign90, ign91,      \
    ign92, ign93, ign94, ign95, ign96, ign97, ign98, ign99, ign100, name, ...) \
    name

#define KL_FOR_EACH(action, ...)                                               \
    KL_MSVC_EXP(KL_MATCH_ARGS(                                                 \
        _0, __VA_ARGS__, KL_FE_100, KL_FE_99, KL_FE_98, KL_FE_97, KL_FE_96,    \
        KL_FE_95, KL_FE_94, KL_FE_93, KL_FE_92, KL_FE_91, KL_FE_90, KL_FE_89,  \
        KL_FE_88, KL_FE_87, KL_FE_86, KL_FE_85, KL_FE_84, KL_FE_83, KL_FE_82,  \
        KL_FE_81, KL_FE_80, KL_FE_79, KL_FE_78, KL_FE_77, KL_FE_76, KL_FE_75,  \
        KL_FE_74, KL_FE_73, KL_FE_72, KL_FE_71, KL_FE_70, KL_FE_69, KL_FE_68,  \
        KL_FE_67, KL_FE_66, KL_FE_65, KL_FE_64, KL_FE_63, KL_FE_62, KL_FE_61,  \
        KL_FE_60, KL_FE_59, KL_FE_58, KL_FE_57, KL_FE_56, KL_FE_55, KL_FE_54,  \
        KL_FE_53, KL_FE_52, KL_FE_51, KL_FE_50, KL_FE_49, KL_FE_48, KL_FE_47,  \
        KL_FE_46, KL_FE_45, KL_FE_44, KL_FE_43, KL_FE_42, KL_FE_41, KL_FE_40,  \
        KL_FE_39, KL_FE_38, KL_FE_37, KL_FE_36, KL_FE_35, KL_FE_34, KL_FE_33,  \
        KL_FE_32, KL_FE_31, KL_FE_30, KL_FE_29, KL_FE_28, KL_FE_27, KL_FE_26,  \
        KL_FE_25, KL_FE_24, KL_FE_23, KL_FE_22, KL_FE_21, KL_FE_20, KL_FE_19,  \
        KL_FE_18, KL_FE_17, KL_FE_16, KL_FE_15, KL_FE_14, KL_FE_13, KL_FE_12,  \
        KL_FE_11, KL_FE_10, KL_FE_9, KL_FE_8, KL_FE_7, KL_FE_6, KL_FE_5,       \
        KL_FE_4, KL_FE_3, KL_FE_2, KL_FE_1, KL_FE_0)(action, __VA_ARGS__))

#define KL_IDENTITY(x) x
#define KL_UNPACK_IF_TUPLE(...) __VA_ARGS__
#define KL_CHOOSE_FOR_PAIR_IMPL(arg1, arg2_or_pair_macro,                      \
                                pair_or_non_pair_macro, ...)                   \
    pair_or_non_pair_macro
#define KL_CHOOSE_FOR_PAIR(...)                                                \
    KL_MSVC_EXP(KL_CHOOSE_FOR_PAIR_IMPL(__VA_ARGS__))

// Invokes macro from 2nd or 3rd argument depending if first argument is a pair
// or a single value
#define KL_IF_PAIR(v, pair_macro, non_pair_macro)                              \
    KL_CHOOSE_FOR_PAIR(KL_UNPACK_IF_TUPLE v, pair_macro, non_pair_macro, 0)(v)

#define KL_MAKE_IDENTITY_PAIR(x) (x, x)

// Replaces single x values with (x, x) pair or leave it if it is already a pair
#define KL_MAKE_PAIR(x) KL_IF_PAIR(x, KL_IDENTITY, KL_MAKE_IDENTITY_PAIR)

#define KL_MAKE_PAIRS(...) KL_FOR_EACH(KL_MAKE_PAIR, __VA_ARGS__)
