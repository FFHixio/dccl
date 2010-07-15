// copyright 2010 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)    
// 
// this file is part of goby-util, a collection of utility libraries
//
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SCI20100713H
#define SCI20100713H

#include <cmath>

namespace goby
{
    namespace util
    {
        //
        // SCIENCE
        //
    
        // round 'r' to 'dec' number of decimal places
// we want no upward bias so
// round 5 up if odd next to it, down if even
        inline double unbiased_round(double r, double dec)
        {
            double ex = pow(10, dec);
            double final = floor(r * ex);
            double s = (r * ex) - final;

            // remainder less than 0.5 or even number next to it
            if (s < 0.5 || (s==0.5 && !(static_cast<unsigned long>(final)&1)))
                return final / ex;
            else 
                return (final+1) / ex;
        }

// K.V. Mackenzie, Nine-term equation for the sound speed in the oceans (1981) J. Acoust. Soc. Am. 70(3), pp 807-812
// http://scitation.aip.org/getabs/servlet/GetabsServlet?prog=normal&id=JASMAN000070000003000807000001&idtype=cvips&gifs=yes
        inline double mackenzie_soundspeed(double T, double S, double D)
        {
            return
                1448.96 + 4.591*T - 5.304e-2*T*T + 2.374e-4*T*T*T +
                1.340*(S-35) + 1.630e-2*D+1.675e-7*D*D -
                1.025e-2*T*(S-35)-7.139e-13*T*D*D*D;
        }
    }
}


#endif
