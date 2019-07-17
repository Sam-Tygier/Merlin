/*
 * Merlin++: C++ Class Library for Charged Particle Accelerator Simulations
 * Copyright (c) 2001-2018 The Merlin++ developers
 * This file is covered by the terms the GNU GPL version 2, or (at your option) any later version, see the file COPYING
 * This file is derived from software bearing the copyright notice in merlin4_copyright.txt
 */

#include "ParticleBunch.h"
#include "SynchRadParticleProcess.h"
#include "RingDeltaTProcess.h"
#include "ClosedOrbit.h"
#include "TLASimp.h"

#ifdef DEBUG_CLOSED_ORBIT
#include "NANCheckProcess.h"
#endif

using namespace std;
using namespace TLAS;
using namespace ParticleTracking;

ClosedOrbit::ClosedOrbit(AcceleratorModel* aModel, double refMomentum, const ParticleInfo* pi) :
	theModel(aModel), p0(refMomentum), transverseOnly(false), radiation(false), useFullAcc(false), delta(1.0e-9), tol(
		1.0e-26), max_iter(20), bendscale(0), theTracker(new ParticleTracker)
{
	if(pi)
	{
		particle_info = pi;
	}
}

ClosedOrbit::~ClosedOrbit()
{
	delete theTracker;
}

void ClosedOrbit::AddProcess(ParticleBunchProcess* aProcess)
{
	theTracker->AddProcess(aProcess);
}

void ClosedOrbit::TransverseOnly(bool flag)
{
	transverseOnly = flag;
}

void ClosedOrbit::Radiation(bool flag)
{
	radiation = flag;

	if(radiation)
	{
		SetRadNumSteps(1);
	}
}

void ClosedOrbit::FullAcceleration(bool flag)
{
	useFullAcc = flag;
}

void ClosedOrbit::SetDelta(double new_delta)
{
	delta = new_delta;
}

void ClosedOrbit::SetTolerance(double tolerance)
{
	tol = tolerance;
}

void ClosedOrbit::SetMaxIterations(int max_iterations)
{
	max_iter = max_iterations;
}

void ClosedOrbit::SetRadStepSize(double rad_stepsize)
{
	radstepsize = rad_stepsize;
	radnumsteps = 0;
}

void ClosedOrbit::SetRadNumSteps(int rad_numsteps)
{
	radnumsteps = rad_numsteps;
	radstepsize = 0;
}

void ClosedOrbit::ScaleBendPathLength(double scale)
{
	bendscale = scale;
}

void ClosedOrbit::FindClosedOrbit(PSvector& particle, int ncpt)
{
	const int cpt = transverseOnly ? 4 : 6;

	ParticleBunch bunch(p0, 1.0, particle_info);
	int k = 0;
	for(k = 0; k < cpt + 1; k++)
	{
		bunch.push_back(particle);
	}

	//	ParticleTracker tracker(theModel->GetRing(ncpt), &bunch, true);
	theTracker->SetRing(theModel->GetRing(ncpt));
	theTracker->SetInitialBunch(&bunch, false);

	// move the declaration of these pointers
	// outside respective if blocks so we
	// can check them for non-null status
	// at the end of the method
	SynchRadParticleProcess* srproc = nullptr;
	RingDeltaTProcess* ringdt = nullptr;
#ifdef DEBUG_CLOSED_ORBIT
	NANCheckProcess* NANproc = nullptr;
#endif

	if(radiation)
	{
		srproc = new SynchRadParticleProcess(1);

		if(radstepsize == 0)
		{
			srproc->SetNumComponentSteps(radnumsteps);
		}
		else
		{
			srproc->SetMaxComponentStepSize(radstepsize);
		}

		srproc->AdjustBunchReferenceEnergy(false);
		theTracker->AddProcess(srproc);
	}

	if(bendscale != 0)
	{
		ringdt = new RingDeltaTProcess(2);
		ringdt->SetBendScale(bendscale);
		theTracker->AddProcess(ringdt);
	}

	RealVector g(cpt);
	RealMatrix dg(cpt);
	w = 1.0;
	iter = 1;

#ifdef DEBUG_CLOSED_ORBIT
	cout << "Finding closed orbit:" << endl;
	NANproc = new NANCheckProcess();
	NANproc->SetDetailed();
	theTracker->AddProcess(NANproc);
#endif

	while((w > tol) && (iter < max_iter))
	{
		// Note that 'bunch' always corresponds to the initial bunch
		// (it is *not* the tracked bunch)
		// Note that the *first* particle is the reference ray
		ParticleBunch::iterator ip;

		k = 0;
		for(ip = bunch.begin(); ip != bunch.end(); ip++, k++)
		{
			*ip = particle;
			if(k > 0)
			{
				(*ip)[k - 1] += delta;
			}
		}

		theTracker->Track(&bunch);

		ip = bunch.begin();
		const Particle& p_ref = *ip++; // reference particle

#ifdef DEBUG_CLOSED_ORBIT
		cout << "After tracking:" << endl;
		cout << p_ref << endl;
#endif

		for(k = 0; k < cpt; k++, ip++)
		{
			for(int m = 0; m < cpt; m++)
			{
				dg(m, k) = ((*ip)[m] - p_ref[m]) / delta;
			}
			dg(k, k) -= 1.;
			g(k) = p_ref[k] - particle[k];
		}

		SVDMatrix<double> invdg(dg);
		g = invdg(g);
		for(int row = 0; row < cpt; row++)
		{
			particle[row] -= g(row);
		}

		w = g * g; // dot product!
		iter++;

#ifdef DEBUG_CLOSED_ORBIT
		cout << p_ref << endl;
#endif

	}

	// clean-up
	// To prevent multiple processes building up
	// in theTracker, we remove the two processes
	// created in this routine (if necessary)
	if(srproc)
	{
		theTracker->RemoveProcess(srproc);
	}
	if(ringdt)
	{
		theTracker->RemoveProcess(ringdt);
		delete ringdt;
	}

#ifdef DEBUG_CLOSED_ORBIT
	theTracker->RemoveProcess(NANproc);
	cout << "Found closed orbit: " << w << endl;
	cout << particle << endl;
#endif
}

void ClosedOrbit::FindRMSOrbit(PSvector& particle)
{
	ParticleTracker tracker(theModel->GetBeamline(), particle, p0);

	double len = 0.0;
	double dl = 0.0;
	PSvector prev = particle;
	PSvector rms(0);

	tracker.InitStepper();
	bool loop = true;
	do
	{
		dl = tracker.GetCurrentComponent().GetLength();
		loop = tracker.StepComponent();

		PSvector pres = tracker.GetTrackedBunch().FirstParticle();

		for(int m = 0; m < 6; m++)
		{
			rms[m] += dl * (pres[m] + prev[m]) * (pres[m] + prev[m]) / 4;
		}

		prev = pres;
		len += dl;
	} while(loop);

	for(int m = 0; m < 6; m++)
	{
		particle[m] = sqrt(rms[m] / len);
	}
}
