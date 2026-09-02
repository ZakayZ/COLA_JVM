%module cola_jvm_native

%{
#include <cstdint>
#include <exception>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
#include <EventData.hh>
#include <LorentzVector.hh>
%}

// No C++ exception may cross a JNI boundary. Convert failures from every
// SWIG-generated native method into the closest standard Java exception.
%exception {
  try {
    $action
  } catch (const std::bad_alloc &error) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, error.what());
    return $null;
  } catch (const std::out_of_range &error) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, error.what());
    return $null;
  } catch (const std::invalid_argument &error) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIllegalArgumentException, error.what());
    return $null;
  } catch (const std::exception &error) {
    SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, error.what());
    return $null;
  } catch (...) {
    SWIG_JavaThrowException(jenv, SWIG_JavaUnknownError, "Unknown native COLA exception");
    return $null;
  }
}

%ignore cola::Vector3;
%ignore cola::RotateUz;
%ignore cola::operator<<;
%ignore cola::LorentzVectorImpl::operator[];
%ignore cola::LorentzVectorImpl::operator+=;
%ignore cola::LorentzVectorImpl::operator-=;
%ignore cola::LorentzVectorImpl::operator*=;
%ignore cola::LorentzVectorImpl::operator/=;
%ignore cola::LorentzVectorImpl::Boost;
%ignore cola::LorentzVectorImpl::BoostAxisRapidity;
%ignore cola::LorentzVectorImpl::SpatialPart;
%ignore cola::LorentzVectorImpl::Mag2;
%ignore cola::LorentzVectorImpl::Mag;

%include <stdint.i>
%include <std_pair.i>
%include <std_vector.i>

%include <LorentzVector.hh>
%template(LorentzVector) cola::LorentzVectorImpl<double>;

%extend cola::LorentzVectorImpl<double> {
  LorentzVectorImpl(double e, double x, double y, double z) {
    auto *vector = new cola::LorentzVector();
    vector->e = e;
    vector->x = x;
    vector->y = y;
    vector->z = z;
    return vector;
  }
}

%rename(PRODUCED) cola::ParticleClass::kProduced;
%rename(ELASTIC_A) cola::ParticleClass::kElasticA;
%rename(ELASTIC_B) cola::ParticleClass::kElasticB;
%rename(NON_ELASTIC_A) cola::ParticleClass::kNonelasticA;
%rename(NON_ELASTIC_B) cola::ParticleClass::kNonelasticB;
%rename(SPECTATOR_A) cola::ParticleClass::kSpectatorA;
%rename(SPECTATOR_B) cola::ParticleClass::kSpectatorB;

%rename(pdgCode) cola::Particle::pdg_code;
%rename(particleClass) cola::Particle::p_class;
%rename(EventInitialState) cola::EventIniState;
%rename(pdgCodeA) cola::EventIniState::pdg_code_a;
%rename(pdgCodeB) cola::EventIniState::pdg_code_b;
%rename(pzA) cola::EventIniState::pz_a;
%rename(pzB) cola::EventIniState::pz_b;
%rename(sectNN) cola::EventIniState::sect_nn;
%rename(impactParameter) cola::EventIniState::b;
%rename(numColl) cola::EventIniState::num_coll;
%rename(numCollPP) cola::EventIniState::num_coll_pp;
%rename(numCollPN) cola::EventIniState::num_coll_pn;
%rename(numCollNN) cola::EventIniState::num_coll_nn;
%rename(numPart) cola::EventIniState::num_part;
%rename(numPartA) cola::EventIniState::num_part_a;
%rename(numPartB) cola::EventIniState::num_part_b;
%rename(phiRotA) cola::EventIniState::phi_rot_a;
%rename(thetaRotA) cola::EventIniState::theta_rot_a;
%rename(phiRotB) cola::EventIniState::phi_rot_b;
%rename(thetaRotB) cola::EventIniState::theta_rot_b;
%rename(initialStateParticles) cola::EventIniState::ini_state_particles;
%rename(initialState) cola::EventData::ini_state;

%typemap(javacode) cola::EventData %{
  /** Internal COLA bridge: transfers a native event into an owning Java proxy. */
  static EventData ownNative(long address) {
    if (address == 0) {
      throw new IllegalArgumentException("Native EventData address must not be zero");
    }
    return new EventData(address, true);
  }

  /** Returns whether this proxy still refers to a native event. */
  public boolean isValid() {
    return getCPtr(this) != 0;
  }

  /** Internal unsafe-filter bridge: transfers this event back to COLA. */
  long releaseToNative() {
    return swigRelease(this);
  }

  /** Internal COLA bridge: returns the wrapped native address without changing ownership. */
  long nativeAddress() {
    return getCPtr(this);
  }
%}

%include <EventData.hh>

%template(AZ) std::pair<uint16_t, uint16_t>;
%template(EventParticles) std::vector<cola::Particle>;
