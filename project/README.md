# Architecture Project Report

<aside>
💡

Graduate Student Project:

</aside>

## RISC-V Simulator: Dynamic Branch Prediction and Data Caches

### Branch Predictor Accuracy:

| Test Case | No BP | BP - 1 Level | BP _ 2 Level | Better Predictor |
| --- | --- | --- | --- | --- |
| binary_search.out | 60% | 60% | 60% | - |
| binary_search_itr.out | 51.67% | 61.67% | 73.33% | 2-Level |
| colliding_cache.out | 0 | 0 | 0 | - |
| colliding_cache_itr.out | 10% | 70% | 20% | 1-Level |
| correlation.out | 45.45% | 45.45% | 79.55% | 2-Level |
| correlation_itr.out | 44.67% | 48.67% | 89.78% | 2-Level |
| derivative.out | 3.23% | 90.32% | 74.19% | 1-Level |
| derivative_itr.out | 3.44% | 95.31% | 91.56% | 1-Level |
| derivative_unrolled.out | 25% | 25% | 25% | - |
| endian_reverse.out | 25% | 25% | 25% | - |
| endian_reverse_itr.out | 22% | 70% | 74% | 2-Level |
| fibonacci.out | 10% | 70% | 20% | 1-Level |
| fibonacci_itr.out | 9.39% | 89.39% | 85.76% | 1-Level |
| matrix_mul.out | 68.18% | 68.18% | 68.18% | - |
| matrix_mul_itr.out | 48.71% | 50.65% | 83.23% | 2-Level |
| nested_loops.out | 28.12% | 56.25% | 65.62% | 2-Level |
| nested_loops_itr.out | 27.58% | 70.30% | 91.82% | 2-Level |
| twos_compliment.out | 100% | 100% | 100% | - |
| twos_compliment_itr.out | 55% | 85% | 75% | 1-Level |
| wordsearch.out | 91.67% | 91.67% | 91.67% | - |
| wordsearch_itr.out | 85.38% | 90% | 90% | 1-Level, 2-Level |

**Observation for each test case:** 

1. For binary_search.out, all three configurations show equal 60% accuracy, indicating the branch patterns are relatively simple and don't benefit from more sophisticated prediction.
2. binary_search_itr.out shows clear improvement with better predictors - 2-Level BP performs best at 73.33% due to its ability to better handle nested loop patterns in the iterative implementation.
3. colliding_cache.out shows 0% across all configurations, suggesting highly unpredictable branch behavior, while its iterative version benefits most from 1-Level BP (70%).
4. correlation.out and correlation_itr.out both show significant improvements with 2-Level BP (79.55% and 89.78% respectively) due to better pattern recognition in complex calculations.
5. derivative.out and derivative_itr.out perform best with 1-Level BP (90.32% and 95.31%), suggesting simple, consistent branch patterns.
6. For nested_loops.out and nested_loops_itr.out, 2-Level BP shows clear advantages (65.62% and 91.82%) by better handling the multiple loop iterations.
7. The iterative implementations generally show better prediction rates than their recursive counterparts, as they tend to have more regular branch patterns.

---

### Cache Accuracy:

| Test Case | Direct-Mapped | 2-way assoc | 4-way assoc | Better Cache Implementation |
| --- | --- | --- | --- | --- |
| binary_search.out | 75% | 75% | 75% | Any |
| binary_search_itr.out | 97.50% | 97.50% | 97.50% | Any |
| colliding_cache.out | 78.57% | 78.57% | 71.43% | DM, 2-way |
| colliding_cache_itr.out | 97.86% | 97.86% | 87.50% | DM, 2-way |
| correlation.out | 0% | 0% | 0% | - |
| correlation_itr.out | 0 | 0 | 0 | - |
| derivative.out | 92.06% | 92.06% | 92.06% | Any |
| derivative_itr.out | 99.21% | 99.21% | 99.21% | Any |
| derivative_unrolled.out | 93.94% | 93.94% | 93.94% | Any |
| endian_reverse.out | 50% | 50% | 50% | Any |
| endian_reverse_itr.out | 95% | 95% | 95% | Any |
| fibonacci.out | 0 | 0 | 0 | - |
| fibonacci_itr.out | 0 | 0 | 0 | - |
| matrix_mul.out | 95% | 95% | 95% | Any |
| matrix_mul_itr.out | 99.50% | 99.50% | 99.50% | Any |
| nested_loops.out | 91.67% | 91.67% | 91.67% | Any |
| nested_loops_itr.out | 99.17% | 99.17% | 99.17% | Any |
| twos_compliment.out | 0 | 0 | 0 | - |
| twos_compliment_itr.out | 0 | 0 | 0 | - |
| wordsearch.out | 83.33% | 83.33% | 83.33% | Any |
| wordsearch_itr.out | 98.33% | 98.33% | 98.33% | Any |

**Observation for each test case:** 

Based on the cache performance data, several key configurations show distinct advantages:

1. For most test cases (binary_search, derivative, matrix_mul, etc.), all three cache configurations perform equally well, suggesting that the memory access patterns are simple and sequential enough that even direct-mapped caching is sufficient.
2. Colliding_cache and its iterative version specifically benefit from Direct-Mapped and 2-way associative caches (78.57% and 97.86% hit rates) compared to 4-way associative (71.43% and 87.50%), indicating that simpler cache organizations handle their access patterns more efficiently.
3. Iterative implementations consistently show superior cache hit rates compared to their recursive counterparts (e.g., binary_search_itr.out at 97.50% vs binary_search.out at 75%), due to their more predictable and localized memory access patterns.
4. Some algorithms (correlation, fibonacci, twos_complement) show 0% hit rates across all configurations, suggesting their memory access patterns exceed cache capacity or have poor spatial/temporal locality.

---

### 2-Level BP and Caches Enabled - showing cache hit rate here

| Test Case | 2-way | 4-way | Better Implementation |
| --- | --- | --- | --- |
| binary_search.out | 75% | 75% | Any |
| binary_search_itr.out | 97.50% | 97.50% | Any |
| colliding_cache.out | 78.57% | 71.43% | 2-Way |
| colliding_cache_itr.out | 97.86% | 87.50% | 2-Way |
| correlation.out | 0 | 0 | - |
| correlation_itr.out | 0 | 0 | - |
| derivative.out | 92.06% | 92.06% | Any |
| derivative_itr.out | 99.21% | 99.21% | Any |
| derivative_unrolled.out | 93.94% | 93.94% | Any |
| endian_reverse.out | 50% | 50% | Any |
| endian_reverse_itr.out | 95% | 95% | Any |
| fibonacci.out | 0 | 0 | - |
| fibonacci_itr.out | 0 | 0 | - |
| matrix_mul.out | 95% | 95% | Any |
| matrix_mul_itr.out | 99.50% | 99.50% | Any |
| nested_loops.out | 91.67% | 91.67% | Any |
| nested_loops_itr.out | 99.17% | 99.17% | Any |
| twos_compliment.out | 0 | 0 | - |
| twos_compliment_itr.out | 0 | 0 | - |
| wordsearch.out | 83.33% | 83.33% | - |
| wordsearch_itr.out | 98.33% | 98.33% | - |

**Observation for each test case:**

When analyzing the performance with 2-Level Branch Prediction enabled and different cache configurations, several key patterns emerge:

1. Most test cases show identical performance between 2-way and 4-way associative caches, suggesting that the increased associativity doesn't provide additional benefits for these workloads.
2. The colliding_cache implementations specifically benefit from 2-way associative cache (78.57% and 97.86%) compared to 4-way (71.43% and 87.50%), indicating that lower associativity better handles their specific memory access patterns.
3. Iterative versions consistently outperform their recursive counterparts across all configurations, with significantly higher cache hit rates (e.g., binary_search_itr.out at 97.50% vs binary_search.out at 75%), demonstrating better locality of reference.

---

