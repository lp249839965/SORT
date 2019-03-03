/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2019 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "hair.h"
#include "sampler/sample.h"
#include "core/samplemethod.h"
#include "fresnel.h"
#include "math/utils.h"

static inline void Ap( const float cosThetaO , const float eta , const float sinGammaT , const Spectrum& T , Spectrum ap[] ){
    const auto cosGammaO = ssqrt( 1.0f - SQR( sinGammaT ) );
    const auto cosTheta = cosThetaO * cosGammaO;
    const auto f = DielectricFresnel( cosTheta , 1.0f , eta );

    ap[0] = f;
    ap[1] = T * SQR( 1.0f - f );
    for( auto i = 2 ; i < PMAX ; ++i )
        ap[i] = ap[i-1] * T * f;
    ap[PMAX] = ap[PMAX - 1] * f * T / ( 1.0f - T * f );
}

static inline float I0(const float x) {
    auto val = 0.0f;
    auto x2i = 1.0f;
    int64_t ifact = 1;
    auto i4 = 1;
    for (int i = 0; i < 10; ++i) {
        if (i > 1) ifact *= i;
        val += x2i / (i4 * SQR(ifact));
        x2i *= x * x;
        i4 *= 4;
    }
    return val;
}

static inline float LogI0(float x) {
    return (x > 12) ? x + 0.5 * (-log(TWO_PI) + log(1 / x) + 1 / (8 * x)) : log(I0(x));
}

static inline float Mp( const float cosThetaI , const float cosThetaO , const float sinThetaI , const float sinThetaO , const float v ){
    const auto a = cosThetaI * cosThetaO / v;
    const auto b = sinThetaI * sinThetaO / v;
    return (v <= .1) ? (exp(LogI0(a) - b - 1 / v + 0.6931f + log(1 / (2 * v)))) : (exp(-b) * I0(a)) / (sinh(1 / v) * 2 * v);
}

static inline float Phi( const int p , const float gammaO , const float gammaT ){
    return 2.0f * p * gammaT - 2.0f * gammaO + p * PI;
}

static inline float Logistic( float x , const float scale ){
    x = abs(x);
    return exp( -x / scale ) / ( scale * SQR( 1.0f + exp( -x / scale ) ) );
}

static inline float LogisticCDF( const float x , const float scale ){
    return 1.0f / ( 1.0f + exp( -x / scale ) );
}

static inline float TrimmedLogistic( const float x , const float scale , const float a , const float b ){
    return Logistic( x , scale ) / ( LogisticCDF( b , scale ) - LogisticCDF( a , scale ) );
}

static inline float SampleTrimmedLogistic(const float r, const float scale, const float a, const float b) {
    const auto k = LogisticCDF(b, scale) - LogisticCDF(a, scale);
    const auto x = -scale * log(1 / (r * k + LogisticCDF(a, scale)) - 1);
    return clamp(x, a, b);
}

static inline float Np( const float phi , const int p , const float scale , const float gammaO , const float gammaT ){
    float dphi = phi - Phi( p , gammaO , gammaT );
    while( dphi > PI ) dphi -= TWO_PI;
    while( dphi < -PI ) dphi += TWO_PI;
    return TrimmedLogistic( dphi, scale, -PI, PI);
}

static inline void ComputeApPdf( const float cosGammaO , const float cosThetaO , const float sinThetaO , const float eta , const Spectrum sigma, float pdf[] ){
    const auto sinThetaT = sinThetaO / eta;
    const auto cosThetaT = ssqrt( 1.0f - SQR( sinThetaT ) );

    const auto etap = sqrt( SQR( eta ) - SQR( sinThetaO ) ) / cosThetaO;

    const auto sinGammaO = ssqrt( 1.0f - SQR( cosGammaO ) );

    const auto sinGammaT = sinGammaO / etap;
    const auto cosGammaT = ssqrt( 1.0f - SQR(sinGammaT) );
    const auto gammaT = asin( clamp( sinGammaT , -1.0f , 1.0f ) );
    
    const auto T = sigma * ( -2.0f * cosGammaT / cosThetaT );
    const auto expT = T.Exp();
    
    Spectrum ap[PMAX + 1];
    Ap( cosThetaO , eta , sinGammaT , expT , ap );

    auto sumY = 0.0f;
    for( auto i = 0 ; i <= PMAX ; ++i )
        sumY += ap[i].GetIntensity();

    for (int i = 0; i <= PMAX; ++i) 
        pdf[i] = ap[i].GetIntensity() / sumY;
}

Hair::Hair(const Spectrum& absorption, const float lRoughness, const float aRoughness, const float ior, const Spectrum& weight, bool doubleSided)
        : Bxdf(weight, (BXDF_TYPE)(BXDF_DIFFUSE | BXDF_REFLECTION), Vector(0.0f,1.0f,0.0f), doubleSided) ,
        m_sigma(absorption), m_lRoughness(lRoughness), m_aRoughness(aRoughness), m_eta(ior){
    m_v[0] = SQR(0.726f * m_lRoughness + 0.812f * SQR(m_lRoughness) + 3.7f * Pow<20>(m_lRoughness));
    m_v[1] = .25 * m_v[0];
    m_v[2] = 4 * m_v[0];
    for (int p = 3; p <= PMAX; ++p)
        m_v[p] = m_v[2];

    // Hard coded tilt angle, 2 degrees by default.
    constexpr auto alpha = 2.0f / 180.0f;
    m_sin2kAlpha[0] = sin( alpha );
    m_cos2kAlpha[0] = ssqrt( 1.0f - SQR( m_sin2kAlpha[0] ) );
    for( auto i = 1 ; i < PMAX ; ++i ){
        m_sin2kAlpha[i] = 2 * m_cos2kAlpha[i-1] * m_sin2kAlpha[i-1];
        m_cos2kAlpha[i] = SQR( m_cos2kAlpha[i-1] ) - SQR( m_sin2kAlpha[i-1] );
    }

    constexpr auto SqrtPiOver8 = 0.626657069f; //sqrt( PI / 8.0f );
    m_scale = SqrtPiOver8 * (0.265f * m_aRoughness + 1.194f * SQR(m_aRoughness) + 5.372f * Pow<22>(m_aRoughness));

    m_etaSqr = SQR( m_eta );
}

Spectrum Hair::f( const Vector& wo , const Vector& wi ) const{
    const auto sinThetaO = wo.x;
    const auto cosThetaO = ssqrt( 1.0f - SQR(sinThetaO) );
    const auto phiO = atan2(wo.y, wo.z);

    const auto sinThetaI = wi.x;
    const auto cosThetaI = ssqrt( 1.0f - SQR(sinThetaI) );
    const auto phiI = atan2(wi.y, wi.z);

    const auto sinThetaT = sinThetaO / m_eta;
    const auto cosThetaT = ssqrt( 1.0f - SQR(sinThetaT) );

    // Modified index of refraction.
    // 'Light Scattering from Human Hair Fibers'
    // http://www.graphics.stanford.edu/papers/hair/hair-sg03final.pdf
    const auto etap = sqrt( m_etaSqr - SQR( sinThetaO ) ) / cosThetaO;

    const auto cosGammaO = wo.y;
    const auto sinGammaO = ssqrt( 1.0f - SQR( cosGammaO ) );
    const auto gammaO = asin( clamp( sinGammaO , -1.0f , 1.0f ) );

    const auto sinGammaT = sinGammaO / etap;
    const auto cosGammaT = ssqrt( 1.0f - SQR(sinGammaT) );
    const auto gammaT = asin( clamp( sinGammaT , -1.0f , 1.0f ) );

    const auto T = m_sigma * ( -2.0f * cosGammaT / cosThetaT );
    const auto expT = T.Exp();
    const auto phi = phiI - phiO;

    Spectrum ap[PMAX + 1];
    Ap( cosThetaO , m_eta , sinGammaT , expT , ap );

    Spectrum fsum(0.0f);
    for( auto p = 0 ; p < PMAX ; ++p ){
        float sinThetaIp , cosThetaIp;
        if( p == 0 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[1] + cosThetaI * m_sin2kAlpha[1];
            cosThetaIp = cosThetaI * m_cos2kAlpha[1] - sinThetaI * m_sin2kAlpha[1];
        }else if( p == 1 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[0] - cosThetaI * m_sin2kAlpha[0];
            cosThetaIp = cosThetaI * m_cos2kAlpha[0] + sinThetaI * m_sin2kAlpha[0];
        }else if( p == 2 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[2] - cosThetaI * m_sin2kAlpha[2];
            cosThetaIp = cosThetaI * m_cos2kAlpha[2] + sinThetaI * m_sin2kAlpha[2];
        }else{
            sinThetaIp = sinThetaI;
            cosThetaIp = cosThetaI;
        }
        
        cosThetaIp = abs( cosThetaIp );
        fsum += Mp( cosThetaIp , cosThetaO , sinThetaIp , sinThetaO , m_v[p] ) * ap[p] * Np( phi, p, m_scale, gammaO, gammaT );
    }
    fsum += Mp( cosThetaI, cosThetaO, sinThetaI, sinThetaO, m_v[PMAX] ) * ap[PMAX] * INV_TWOPI;

    return fsum;
}

Spectrum Hair::sample_f(const Vector& wo, Vector& wi, const BsdfSample& bs, float* pPdf) const {
    const auto sinThetaO = wo.x;
    const auto cosThetaO = ssqrt( 1.0f - SQR( sinThetaO ) );
    const auto phiO = atan2( wo.y , wo.z );

    float apPdf[PMAX + 1] = {0.0f};
    ComputeApPdf( wo.y , cosThetaO , sinThetaO , m_eta , m_sigma , apPdf );
    auto r = sort_canonical();
    auto p = 0;
    for( ; p < PMAX ; ++p ){
        if( r < apPdf[p] ) break;
        r -= apPdf[p];
    }

    r = sort_canonical();
    const auto cosTheta = 1.0f + m_v[p] * log( r + ( 1.0f - r ) * exp( -2.0f / m_v[p] ) );
    const auto sinTheta = ssqrt( 1.0f - SQR( cosTheta ) );
    const auto cosPhi = cos( TWO_PI * sort_canonical() );
    auto sinThetaI = -cosTheta * sinThetaO + sinTheta * cosPhi * cosThetaO;
    auto cosThetaI = ssqrt( 1.0f - SQR( sinThetaI ) );

    auto sinThetaIp = sinThetaI;
    auto cosThetaIp = cosThetaI;
    if( p == 0 ){
        sinThetaIp = sinThetaI * m_cos2kAlpha[1] + cosThetaI * m_sin2kAlpha[1];
        cosThetaIp = cosThetaI * m_cos2kAlpha[1] - sinThetaI * m_sin2kAlpha[1];
    }else if( p == 1 ){
        sinThetaIp = sinThetaI * m_cos2kAlpha[0] - cosThetaI * m_sin2kAlpha[0];
        cosThetaIp = cosThetaI * m_cos2kAlpha[0] + sinThetaI * m_sin2kAlpha[0];
    }else if( p == 2 ){
        sinThetaIp = sinThetaI * m_cos2kAlpha[2] - cosThetaI * m_sin2kAlpha[2];
        cosThetaIp = cosThetaI * m_cos2kAlpha[2] + sinThetaI * m_sin2kAlpha[2];
    }

    sinThetaI = sinThetaIp;
    cosThetaI = cosThetaIp;

    const auto etap = sqrt( m_etaSqr - SQR( sinThetaO ) ) / cosThetaO;
    const auto cosGammaO = wo.y;
    const auto sinGammaO = ssqrt( 1.0f - SQR( cosGammaO ) );
    const auto gammaO = asin( clamp( sinGammaO , -1.0f , 1.0f ) );

    const auto sinGammaT = sinGammaO / etap;
    const auto gammaT = asin( clamp( sinGammaT , -1.0f , 1.0f ) );
    const auto dphi = ( p < PMAX ) ? Phi( p , gammaO , gammaT ) + SampleTrimmedLogistic( sort_canonical() , m_scale , -PI , PI ) : TWO_PI * sort_canonical();

    const auto phiI = phiO + dphi;
    wi = Vector3f( sinThetaI , cosThetaI * sin( phiI ) , cosThetaI * cos( phiI ) );

    if( pPdf ){
        *pPdf = 0.0f;

        for( auto p = 0 ; p < PMAX ; ++p ){
            float sinThetaIp , cosThetaIp;
            if( p == 0 ){
                sinThetaIp = sinThetaI * m_cos2kAlpha[1] + cosThetaI * m_sin2kAlpha[1];
                cosThetaIp = cosThetaI * m_cos2kAlpha[1] - sinThetaI * m_sin2kAlpha[1];
            }else if( p == 1 ){
                sinThetaIp = sinThetaI * m_cos2kAlpha[0] - cosThetaI * m_sin2kAlpha[0];
                cosThetaIp = cosThetaI * m_cos2kAlpha[0] + sinThetaI * m_sin2kAlpha[0];
            }else if( p == 2 ){
                sinThetaIp = sinThetaI * m_cos2kAlpha[2] - cosThetaI * m_sin2kAlpha[2];
                cosThetaIp = cosThetaI * m_cos2kAlpha[2] + sinThetaI * m_sin2kAlpha[2];
            }else{
                sinThetaIp = sinThetaI;
                cosThetaIp = cosThetaI;
            }
            
            cosThetaIp = abs( cosThetaIp );
            *pPdf += Mp( cosThetaIp , cosThetaO , sinThetaIp , sinThetaO , m_v[p] ) * apPdf[p] * Np( dphi, p, m_scale, gammaO, gammaT );
        }
        *pPdf += Mp( cosThetaI , cosThetaO , sinThetaI , sinThetaO , m_v[PMAX] ) * apPdf[PMAX] * INV_TWOPI;
    }

    return f( wo , wi );
}

float Hair::pdf( const Vector& wo , const Vector& wi ) const{
    const auto sinThetaO = wo.x;
    const auto cosThetaO = ssqrt( 1.0f - SQR(sinThetaO) );
    const auto phiO = atan2(wo.y, wo.z);

    const auto sinThetaI = wi.x;
    const auto cosThetaI = ssqrt( 1.0f - SQR(sinThetaI) );
    const auto phiI = atan2(wi.y, wi.z);

    const auto sinThetaT = sinThetaO / m_eta;
    const auto cosThetaT = ssqrt( 1.0f - SQR(sinThetaT) );

    const auto etap = sqrt( m_etaSqr - SQR( sinThetaO ) ) / cosThetaO;

    const auto cosGammaO = wo.y;
    const auto sinGammaO = ssqrt( 1.0f - SQR( cosGammaO ) );
    const auto gammaO = asin( clamp( sinGammaO , -1.0f , 1.0f ) );

    const auto sinGammaT = sinGammaO / etap;
    const auto cosGammaT = ssqrt( 1.0f - SQR(sinGammaT) );
    const auto gammaT = asin( clamp( sinGammaT , -1.0f , 1.0f ) );

    float apPdf[PMAX + 1] = {0.0f};
    ComputeApPdf( wo.y , cosThetaO , sinThetaO , m_eta , m_sigma , apPdf );

    auto phi = phiI - phiO;
    auto pdf = 0.0f;
    for( auto p = 0 ; p < PMAX ; ++p ){
        float sinThetaIp , cosThetaIp;
        if( p == 0 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[1] + cosThetaI * m_sin2kAlpha[1];
            cosThetaIp = cosThetaI * m_cos2kAlpha[1] - sinThetaI * m_sin2kAlpha[1];
        }else if( p == 1 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[0] - cosThetaI * m_sin2kAlpha[0];
            cosThetaIp = cosThetaI * m_cos2kAlpha[0] + sinThetaI * m_sin2kAlpha[0];
        }else if( p == 2 ){
            sinThetaIp = sinThetaI * m_cos2kAlpha[2] - cosThetaI * m_sin2kAlpha[2];
            cosThetaIp = cosThetaI * m_cos2kAlpha[2] + sinThetaI * m_sin2kAlpha[2];
        }else{
            sinThetaIp = sinThetaI;
            cosThetaIp = cosThetaI;
        }
        
        cosThetaIp = abs( cosThetaIp );
        pdf += Mp( cosThetaIp , cosThetaO , sinThetaIp , sinThetaO , m_v[p] ) * apPdf[p] * Np( phi, p, m_scale, gammaO, gammaT );
    }
    pdf += Mp( cosThetaI , cosThetaO , sinThetaI , sinThetaO , m_v[PMAX] ) * apPdf[PMAX] * INV_TWOPI;
    return pdf;
}
