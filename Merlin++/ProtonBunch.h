/*
 * Merlin++: C++ Class Library for Charged Particle Accelerator Simulations
 * Copyright (c) 2001-2018 The Merlin++ developers
 * This file is covered by the terms the GNU GPL version 2, or (at your option) any later version, see the file COPYING
 * This file is derived from software bearing the copyright notice in merlin4_copyright.txt
 */

#ifndef ProtonBunch_h
#define ProtonBunch_h 1

#include "ParticleBunch.h"
#include "PhysicalConstants.h"
#include <iostream>

using namespace ParticleTracking;

namespace ParticleTracking
{
class ProtonBunch: public ParticleBunch
{
	static const int ntally = 6;
	int tally[ntally];
	static const ParticleInfo partinfo;

public:

	/**
	 * Constructs a ProtonBunch using the specified momentum,
	 * total charge and the particle array. Note that on exit,
	 * particles is empty.
	 */
	ProtonBunch(double P0, double Q, PSvectorArray& particles) :
		ParticleBunch(P0, Q, particles, &partinfo)
	{
	}

	/**
	 * Read phase space vectors from specified input stream.
	 */
	ProtonBunch(double P0, double Q, std::istream& is) :
		ParticleBunch(P0, Q, is, &partinfo)
	{
	}

	/**
	 * Constructs an empty ProtonBunch with the specified
	 * momentum P0 and charge per macro particle Qm (default =
	 *  +1).
	 */
	ProtonBunch(double P0, double Qm = 1) :
		ParticleBunch(P0, Qm, &partinfo)
	{
	}

	ProtonBunch(size_t np, const ParticleDistributionGenerator & generator, const BeamData& beam,
		ParticleBunchFilter* filter = nullptr) :
		ParticleBunch(np, generator, beam, filter, &partinfo)
	{
	}

	void set()
	{
		for(int i = 0; i < ntally; tally[i++] = 0)
			;
	}

	void report()
	{
		std::cout << " Proton Scatter tallies ";
		for(int i = 0; i < ntally; std::cout << tally[i++] << " ")
			;
		std::cout << std::endl;
	}

}; // end ProtonBunch class

const ParticleInfo ProtonBunch::partinfo = ParticleInfo(PhysicalConstants::ProtonMass, 1);

} // end namespace ParticleTracking
#endif
