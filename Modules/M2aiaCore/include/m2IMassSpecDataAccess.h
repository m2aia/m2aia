/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <MitkM2aiaCoreExports.h>
#include <mitkImage.h>
#include <mitkMessage.h>
#include <vector>

namespace m2
{
  class MITKM2AIACORE_EXPORT IMSImageDataAccess
  {
  public:
    virtual void GrabIntensity(unsigned int index,
                               std::vector<double> &ints,
                               unsigned int sourceIndex = 0) const = 0;
    virtual void GrabMass(unsigned int index,
                          std::vector<double> &mzs,
                          unsigned int sourceIndex = 0) const = 0;
    virtual void GrabIonImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const = 0;
    virtual void GrabSpectrum(unsigned int index,
                              std::vector<double> &mzs,
                              std::vector<double> &ints,
                              unsigned int sourceIndex = 0) const = 0;

    /**
     * \brief Messages to emit
     *
     * Observers should register to this event by calling myLabelSet->AddLabelEvent.AddListener(myObject,
     * MyObject::MyMethod).
     * After registering, myObject->MyMethod() will be called every time a new label has been added to the LabelSet.
     * Observers should unregister by calling myLabelSet->AddLabelEvent.RemoveListener(myObject, MyObject::MyMethod).
     *
     * member variable is not needed to be locked in multi-threaded scenarios since the LabelSetEvent is a typedef for
     * a Message1 object which is thread safe
     */
    mutable mitk::Message<> GrabSpectrumEnd;
    mutable mitk::Message<> GrabSpectrumStart;
    mutable mitk::Message<> GrabImageStart;
    mutable mitk::Message<> GrabImageEnd;
  };

} // namespace mitk