/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "store.hpp"
#include "commands/mighty.hpp"
#include "commands/load.hpp"
#include "commands/exact_imply.hpp"
#include "commands/lut_mapping.hpp"
#include "commands/lut_resyn.hpp"
#include "commands/xmginv.hpp"
#include "commands/exact_m5ig.hpp"
#include "commands/exact_m3ig.hpp"
#include "commands/exact_maj.hpp"
#include "commands/exprsim.hpp"
#include "commands/xmgrw.hpp"
#include "commands/xmgrs.hpp"
#include "commands/cutrw.hpp"
#include "commands/xmgcost.hpp"
#include "commands/magic.hpp"
#include "commands/write_dot.hpp"
#include "commands/cog.hpp"
#include "commands/test_tt.hpp"
#include "commands/test_img.hpp"
#include "commands/test_xagdec.hpp"
#include "commands/imgrw.hpp"
#include "commands/imgff.hpp"
#include "commands/ax.hpp"
#include "commands/xagrs.hpp"
#include "commands/xagopt.hpp"
#include "commands/xmgcost2.hpp"
#include "commands/xagrw.hpp"
#include "commands/xagban.hpp"
#include "commands/stochastic.hpp"
#include "commands/app.hpp"
#include "commands/xmgban.hpp"
#include "commands/heuristic.hpp"
#include "commands/test.hpp"

ALICE_MAIN( also )

