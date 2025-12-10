# BSS-UDF

## Notes on UDF convexity (Raviv & Kolka 2013)

The expected lost-demand cost \(F(I_0)\) in Raviv & Kolka’s birth–death formulation is convex in the initial inventory \(I_0\): the marginal cost of adding a bike (probability of shortage vs. overflow times linear penalties \(p,h\)) is non-decreasing. With perfect evaluation of \(e^{Rt}\) over equal time slices, the UDF produced by this implementation should therefore be convex.

We observed non-convex slopes in practice. The main culprit was a bug in the discretized simulation: the transition matrix for each time slice was built with a **cumulative** \(\Delta t\) (`(timeSlot + 1) * deltaTime`) instead of the slice length (`deltaTime`). That over-inflated later transitions, distorting probabilities and breaking the expected convexity. The code now uses the correct fixed \(\Delta t\) per slice.

Additional practical sources of small non-convexities:
- Discretization/granularity: 30-minute slices approximate time-varying rates; finer slices reduce approximation error.
- Numerical precision: rates and \(\Delta t\) are quantized to 2 decimals for caching; this and floating-point roundoff can slightly perturb monotonic marginal costs. Using tighter quantization or higher precision can further smooth results if needed.
