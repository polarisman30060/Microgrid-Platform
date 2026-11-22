"""Analytical & Fudge-Factor Verification Script for the 10 kW 2-Phase FB-LLC Boost Module.

Hand-Buildable 45.0 kHz SOT-227 + Busbar Baseline:
  - 2 x 5 kW interleaved Full-Bridge LLC (90 deg primary phase shift)
  - Input:  96.0 VDC nominal (91.2 - 100.8 V normal, 84 - 108 V fault)
  - Output: 400.0 VDC split-rail (+200 V / HV_MID=PE=N / -200 V), center-tapped SiC bridge
  - Transformer: Single TDK U93/76/30 (N87), Np:Ns1:Ns2 = 6:13:13, Lm = 12.50 uH (1.52 mm 2-leg shim)
  - Resonant tank: Lr = 2.08 uH (leakage + busbar + tunable external Lr), Cr = 6.00 uF (4 x 1.5 uF screw-lug MKP), fr = 45.05 kHz
  - DIY Wire: 26 AWG twisted bundles (r = 0.202 mm < 0.311 mm skin depth @ 45 kHz)

Run: python hardware/boost_converter/Simulations/fbllc_verify.py
"""

import math

Np = 6
Ns_half = 13
Ns_tot = 2 * Ns_half
n_ps = Np / Ns_tot
step_up = Ns_tot / Np

Lr = 2.08e-6
Cr = 6.00e-6
Lm = 12.50e-6
fr = 1.0 / (2.0 * math.pi * math.sqrt(Lr * Cr))
Zr = math.sqrt(Lr / Cr)
Ln = Lm / Lr
V_drop_sec = 3.0

# TDK U93/76/30 (N87) core parameters
Ae = 840e-6
Ve = 297e-6
Aw = 1520.0
mu0 = 4.0 * math.pi * 1e-7


def fha_gain(f_sw: float, Q: float, ln_val: float = Ln, fr_val: float = fr) -> float:
    fn = f_sw / fr_val
    real_part = 1.0 + (1.0 / ln_val) * (1.0 - 1.0 / (fn ** 2))
    imag_part = Q * (fn - 1.0 / fn)
    return 1.0 / math.sqrt(real_part ** 2 + imag_part ** 2)


def find_peak_gain(Q: float, ln_val: float = Ln, fr_val: float = fr) -> tuple[float, float]:
    best_f, best_m = fr_val, 0.0
    for i in range(350, 1001):
        f = fr_val * (i / 1000.0)
        m = fha_gain(f, Q, ln_val, fr_val)
        if m > best_m:
            best_m, best_f = m, f
    return best_f, best_m


def solve_fsw(M_target: float, Q: float, ln_val: float = Ln, fr_val: float = fr):
    fpk, mpk = find_peak_gain(Q, ln_val, fr_val)
    if M_target > mpk:
        return None, fpk, mpk
    lo, hi = fpk, 3.5 * fr_val
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        if fha_gain(mid, Q, ln_val, fr_val) > M_target:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi), fpk, mpk


def n87_core_loss_w(f_hz: float, B_pk_T: float) -> float:
    pv_kw_m3 = 375.0 * ((f_hz / 100e3) ** 1.46) * ((B_pk_T / 0.200) ** 2.75)
    return pv_kw_m3 * 1e3 * Ve


def main() -> None:
    total_gap_mm = (mu0 * (Np ** 2) * Ae / Lm) * 1e3
    shim_2leg_mm = 0.5 * total_gap_mm
    B_nom = 96.0 / (4.0 * fr * Np * Ae)
    B_30k = 100.8 / (4.0 * 30e3 * Np * Ae)
    ku = (Np * 15.0 + Ns_tot * 3.2) / Aw

    print("=== 10 kW (2 x 5 kW) 45 kHz SOT-227 + BUSBAR FB-LLC VERIFICATION ===")
    print(f"Transformer: TDK U93/76/30 (N87) | Np:Ns1:Ns2 = {Np}:{Ns_half}:{Ns_half} (step-up = {step_up:.3f})")
    print(f"Tank: Lr = {Lr*1e6:.2f} uH, Cr = {Cr*1e6:.2f} uF (screw-lug MKP), Lm = {Lm*1e6:.2f} uH (Ln = {Ln:.2f})")
    print(f"fr = {fr/1e3:.2f} kHz | Zr = {Zr:.3f} Ohm | 2-leg uniform shim = {shim_2leg_mm:.2f} mm | Cu fill ku = {ku:.2f}")
    print(f"Flux: B_pk(45kHz, 96V) = {B_nom*1e3:.1f} mT (P_core ~ {n87_core_loss_w(fr, B_nom):.1f} W) | B_pk(30kHz, 100.8V) = {B_30k*1e3:.1f} mT\n")

    print("=== OPERATING CORNER MATRIX (PER 5 kW PHASE) ===")
    corners = [
        ("Nominal 96.0V -> 400V @ 5.0 kW", 96.0, 400.0, 5000.0),
        ("Min Normal 91.2V -> 400V @ 5.0 kW", 91.2, 400.0, 5000.0),
        ("Min Normal 91.2V -> 415V @ 5.0 kW", 91.2, 415.0, 5000.0),
        ("Max Normal 100.8V -> 400V @ 5.0 kW", 100.8, 400.0, 5000.0),
        ("Max Normal 100.8V -> 380V @ 5.0 kW", 100.8, 380.0, 5000.0),
        ("Light Load 100.8V -> 400V @ 0.5 kW", 100.8, 400.0, 500.0),
        ("Peak Surge 91.2V -> 400V @ 7.5 kW (15kW)", 91.2, 400.0, 7500.0),
        ("UV Fault Edge 84.0V -> 400V @ 5.0 kW", 84.0, 400.0, 5000.0),
    ]
    for label, vin, vout, p in corners:
        Rac = (8.0 / (math.pi ** 2)) * (n_ps ** 2) * (vout ** 2 / p)
        Q = Zr / Rac
        M = (vout + V_drop_sec) / (vin * step_up)
        fsw, fpk, mpk = solve_fsw(M, Q)
        if fsw is not None:
            print(f"{label:42s} | M={M:.3f} (Mpk={mpk:.3f}) | fsw={fsw/1e3:6.2f} kHz | fpk={fpk/1e3:5.2f} kHz | ZVS margin={(fsw-fpk)/1e3:+5.2f} kHz")
        else:
            print(f"{label:42s} | M={M:.3f} > Mpk={mpk:.3f} FAIL")

    print("\n=== DIY HAND-WINDING & PARASITIC FUDGE-FACTOR STRESS TEST (91.2V -> 400V) ===")
    for name, lr_m, lm_m, cr_m in [
        ("Loose hand-wind (Lr+15%, Lm-15%, Cr+5%)", 1.15, 0.85, 1.05),
        ("Tight hand-wind (Lr-15%, Lm+15%, Cr-5%)", 0.85, 1.15, 0.95),
        ("Extra busbar stray (Lr+20%)", 1.20, 1.00, 1.00),
    ]:
        Lr_t, Lm_t, Cr_t = Lr * lr_m, Lm * lm_m, Cr * cr_m
        fr_t = 1.0 / (2.0 * math.pi * math.sqrt(Lr_t * Cr_t))
        Zr_t = math.sqrt(Lr_t / Cr_t)
        Ln_t = Lm_t / Lr_t
        M_min = 403.0 / (91.2 * step_up)
        Rac5 = (8.0 / (math.pi ** 2)) * (n_ps ** 2) * (400.0 ** 2 / 5000.0)
        Rac65 = (8.0 / (math.pi ** 2)) * (n_ps ** 2) * (400.0 ** 2 / 6500.0)
        f5, fpk5, _ = solve_fsw(M_min, Zr_t / Rac5, Ln_t, fr_t)
        f65, fpk65, _ = solve_fsw(M_min, Zr_t / Rac65, Ln_t, fr_t)
        print(f"  {name:40s} | fr={fr_t/1e3:4.1f}k | 5kW: fsw={f5/1e3:4.1f}k (+{(f5-fpk5)/1e3:4.1f}k ZVS) | 6.5kW: fsw={f65/1e3:4.1f}k (+{(f65-fpk65)/1e3:4.1f}k ZVS)")


if __name__ == "__main__":
    main()
