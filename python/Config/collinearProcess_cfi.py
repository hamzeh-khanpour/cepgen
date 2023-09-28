"""@package collinear parton-factorised process objects definition

A collection of useful objects for the definition of a
general collinear parton momentum-factorised process steering card
"""

from .containers_cfi import Module, Parameters

class ProtonFlux:
    """Type of parton (from proton) flux modelling"""
    PhotonElastic = Module('EPAFlux',
        formFactors = Module('StandardDipole')
    )
    PhotonElasticDZ = Module('DreesZeppenfeld')
    PhotonInelastic = Module('EPAFlux',
        formFactors = Module('InelasticNucleon')
    )
    IntegratedPhotonElastic = Module('KTIntegrated',
        ktFlux = Module('BudnevElastic')
    )
    IntegratedPhotonInelastic = Module('KTIntegrated',
        ktFlux = Module('BudnevElastic',
            formFactors = Module('InelasticNucleon')
        )
    )
    LHAPDF = Module('LHAPDF')
    LHAPDFLUXlep = LHAPDF.clone('LHAPDF',
        set = 'LUXlep-NNPDF31_nlo_as_0118_luxqed',
        fromRemnant = False,
    )


class HeavyIonFlux:
    """Type of parton (from heavy ion) flux modelling"""
    PhotonElastic = Module('KTIntegrated',
        ktFlux = Module('BudnevElastic',
            formFactors = Module('HeavyIonDipole')
        )
    )


class ElectronFlux:
    """Type of parton (from electron) flux modelling"""
    PhotonElastic = Module('KTIntegrated',
        ktFlux = Module('BudnevElastic',
            formFactors = Module('PointLikeFermion')
        )
    )


process = Module('collinearProcess',
    ktFactorised = False,
    outKinematics = Parameters(
        #--- cuts on initial-state partons
        q2 = (0., 10.),
        #--- cuts on individual particles defining the central system
        rapidity = (-6., 6.),
    ),
)