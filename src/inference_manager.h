#ifndef INFERENCE_MANAGER_H
#define INFERENCE_MANAGER_H

#include "common.h"
#include "hmm.h"
#include "piecewise_constant_rate_function.h"
#include "conditioned_sfs.h"
#include "transition.h"
#include "transition_bundle.h"
#include "inference_bundle.h"
#include "block_key.h"
#include "marginalize_sfs.h"
#include "ThreadPool.h"

class HMM;

class InferenceManager
{
    public:
    InferenceManager(
            const int,
            const int,
            const std::vector<int>, 
            const std::vector<int*>,
            const std::vector<double>,
            const ConditionedSFS<adouble> *csfs);
    virtual ~InferenceManager() = default;

    void setRho(const double);
    void setTheta(const double);

    void Estep(bool);
    std::vector<adouble> Q();
    std::vector<double> loglik();

    void setParams(const ParameterVector &params);

    bool saveGamma, folded;
    std::vector<double> hidden_states;
    std::map<block_key, Vector<adouble> > emission_probs;
    std::vector<Matrix<double>*> getXisums();
    std::vector<Matrix<double>*> getGammas();
    std::vector<std::map<block_key, Vector<double> >*> getGammaSums();
    Matrix<adouble>& getPi();
    Matrix<adouble>& getTransition();
    Matrix<adouble>& getEmission();
    std::map<block_key, Vector<adouble> >& getEmissionProbs();

    protected:
    typedef std::unique_ptr<HMM> hmmptr;

    // Methods
    void parallel_do(std::function<void(hmmptr &)>);
    template <typename T> std::vector<T> parallel_select(std::function<T(hmmptr &)>);
    void recompute_initial_distribution();
    std::vector<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> > 
        map_obs(const std::vector<int*>&, const std::vector<int>&);
    std::set<std::pair<int, block_key> > fill_targets();
    void populate_emission_probs();
    void do_dirty_work();

    // These methods will differ according to number of populations and must be overridden.
    virtual void recompute_emission_probs() = 0;

    // Passed-in parameters
    std::unique_ptr<const ConditionedSFS<adouble> > csfs;

    // Other members
    const int npop, sfs_dim, M;
    std::vector<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> > obs;
    double theta, rho;
    std::vector<hmmptr> hmms;
    Vector<adouble> pi;
    Matrix<adouble> transition, emission;
    std::vector<block_key> bpm_keys;
    const std::set<std::pair<int, block_key> > targets;
    TransitionBundle tb;
    std::vector<Matrix<adouble> > sfss;

    InferenceBundle ib;
    struct { bool theta, rho, eta; } dirty;

    std::unique_ptr<const PiecewiseConstantRateFunction<adouble> > eta;
};

template <size_t P>
class NPopInferenceManager : public InferenceManager
{
    public:
    NPopInferenceManager(
            const FixedVector<int, P> n,
            const std::vector<int> obs_lengths,
            const std::vector<int*> observations,
            const std::vector<double> hidden_states,
            const ConditionedSFS<adouble> *csfs) :
        InferenceManager(P, (n.array() + 1).prod(), obs_lengths, observations, hidden_states, csfs), n(n) {}

    virtual ~NPopInferenceManager() = default;

    protected:
    // Virtual overrides
    void recompute_emission_probs();

    // Passed-in parameters
    const FixedVector<int, P> n;
};

class OnePopInferenceManager final : public NPopInferenceManager<1>
{
    public:
    OnePopInferenceManager(
            const int n,
            const std::vector<int> obs_lengths,
            const std::vector<int*> observations,
            const std::vector<double> hidden_states) :
        NPopInferenceManager(FixedVector<int, 1>::Constant(n),
                obs_lengths, observations, hidden_states, 
                new OnePopConditionedSFS<adouble>(n, hidden_states.size() - 1)) {}
};

class TwoPopInferenceManager : public NPopInferenceManager<2>
{
    public:
    TwoPopInferenceManager(
            const int n1, const int n2,
            const std::vector<int> obs_lengths,
            const std::vector<int*> observations,
            const std::vector<double> hidden_states) :
        NPopInferenceManager(
                (FixedVector<int, 2>() << n1, n2).finished(),
                obs_lengths, observations, hidden_states, 
                nullptr) {}
                
    void setParams(const ParameterVector &params, std::vector<double*> newSFS)
    {
        InferenceManager::setParams(params);
        csfs.reset(new DummySFS<adouble>(sfs_dim, hidden_states.size() - 1, newSFS));
    }
};

Matrix<adouble> sfs_cython(const int, const ParameterVector, const double, const double, bool);

#endif
