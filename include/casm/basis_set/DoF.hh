#ifndef DOF_HH
#define DOF_HH

#include <string>
#include "casm/CASM_global_definitions.hh"
#include "casm/container/Array.hh"
#include "casm/misc/CASM_math.hh"

namespace CASM {

  class Molecule;
  class Lattice;

  //****************************************************

  // Since we will have different types of DoFs, we make DoF a virtual, or abstract class.
  // The specific implementation will depend on the type of site variable.
  // Not sure if it still makes sense to try to treat continuous and discrete DoFs equivalently
  class DoF {
  protected:
    std::string m_type_name;
    /// dof_ID is a way to distinguish between DoFs with the same name but different identities
    /// dof_ID for now usually refers to the site index of a cluster (e.g., 0, 1, 2 of a triplet)
    /// or an index into the primitive cell neighbor list. Other usage cases may be introduced later
    Index m_dof_ID;
    bool m_ID_lock;
  public:
    DoF(const std::string &_name = "", Index _ID = -1) : m_type_name(_name), m_dof_ID(_ID), m_ID_lock(false) {};
    virtual ~DoF() {}; //= 0;
    //virtual double get_val() const = 0; // get the current value

    const std::string &type_name() const {
      return m_type_name;
    };

    Index ID() const {
      return m_dof_ID;
    };

    void set_ID(Index new_ID) {
      if(m_ID_lock)
        return;
      //std::cout << "DoF " << this << " ID updated from " << ID() << " to " << new_ID << '\n';
      m_dof_ID = new_ID;
    };

    bool is_locked() const {
      return m_ID_lock;
    };

    void lock_ID() {
      m_ID_lock = true;
    };

    void unlock_ID() {
      m_ID_lock = false;
    };

    virtual jsonParser &to_json(jsonParser &json) const = 0;

  };


  jsonParser &to_json(const DoF *dof, jsonParser &json);

  /// This allocates a new object to 'dof'.
  ///   It needs a Lattice in case it is a DiscreteDoF<Molecule>
  ///
  void from_json(DoF *dof, const jsonParser &json, const Lattice &lat);

  //DoF::~DoF() {};

  //****************************************************

  class DiscreteDoF : public DoF {
  protected:
    /// index into domain of the current state, -1 if unspecified
    int m_current_state;

    /// Allows DoF to point to a remote value for faster/easier evaluation
    const int *m_remote_state;

    Index m_sym_rep_ID;

  public:
    DiscreteDoF(): DoF(""), m_current_state(0), m_remote_state(NULL), m_sym_rep_ID(-2) {};

    DiscreteDoF(const std::string &_name, int _current_state = 0):
      DoF(_name), m_current_state(_current_state), m_remote_state(NULL), m_sym_rep_ID(-2) {};

    virtual ~DiscreteDoF() {};

    Index sym_rep_ID()const {
      return m_sym_rep_ID;
    };

    bool is_specified() const {
      return valid_index(m_current_state);
    }

    void invalidate() {
      m_current_state = -1;
    }


    int value() const {
      return m_current_state;
    };

    int remote_value() const {
      assert(m_remote_state && "In DiscreteDoF::remote_value() m_remote_state is NULL.\n");
      return *m_remote_state;
    };

    void register_remote(const int &remote_state) {
      m_remote_state = &remote_state;
    };

    virtual DiscreteDoF *copy() const = 0;
    //      return new DiscreteDoF(*this);
    //};

    virtual Index size()const {
      return 0;
    };

    //John G 050513
    //Make an operator for this
    virtual void set_value(int new_state) {
      m_current_state = new_state;
      return;
    }

    virtual bool operator==(const DiscreteDoF &RHS) const {
      if(value() != RHS.value())
        return false;

      if(size() != RHS.size())
        return false;

      return true;
    }

    void print(std::ostream &out) const {
      out << value();
    }

    virtual jsonParser &to_json(jsonParser &json) const = 0;

  };

  //****************************************************
  template< typename T>
  class OccupantDoF : public DiscreteDoF {
    /**** Inherited from DiscreteDoF ****
    // index into domain of the current state, -1 if unspecified
    //  int m_current_state;

    // Allows DoF to point to a remote value for faster/easier evaluation
    //  const int *m_remote_state;
    **/

    /// Allowed values, assume class T has member 'T.name'
    Array<T> m_domain;

  public:
    OccupantDoF() : DiscreteDoF() { };
    OccupantDoF(const std::string &_name, const Array<T> &_domain, int _current_state = 0) :
      DiscreteDoF(_name, _current_state), m_domain(_domain) { };

    const T &get_occ() const {
      return m_domain[m_current_state];
    };

    DiscreteDoF *copy() const {
      return new OccupantDoF(*this);
    };


    //John G 050513
    //Make an operator for this
    void set_value(int new_state) {
      if(Index(new_state) >= m_domain.size()) {
        std::cerr << "In OccupantDoF::set_value(): Bad Assignment, new_state>=size Go check! EXITING \n";
        assert(0);
        exit(1);
      }
      m_current_state = new_state;
      return;
    }

    void set_current_state(const T &new_state) {
      Index new_index = m_domain.find(new_state);
      if(new_index >= m_domain.size()) {
        std::cerr << "In OccupantDoF::set_current_state(const T &new_state): Bad Assignment, new_state not found in domain. Go check! EXITING \n";
        assert(0);
        exit(1);
      }
      m_current_state = new_index;
      return;
    }

    bool compare(const OccupantDoF &RHS, bool compare_value) const {
      if(compare_value && value() != RHS.value())
        return false;

      if(size() != RHS.size())
        return false;

      for(Index i = 0; i < size(); i++) {
        if((*this)[i] != RHS[i])
          return false;
      }

      return true;
    }

    bool operator==(const OccupantDoF &RHS) const {
      return compare(RHS, true);
    }

    void set_domain(const Array<T> &new_dom) {
      m_domain = new_dom;
      if(new_dom.size() == 1)
        m_current_state = 0;
      else
        m_current_state = -1;
    };

    const Array<T> &get_domain() const {
      return m_domain;
    }

    T &operator[](Index i) {
      return m_domain[i];
    };

    const T &operator[](Index i)const {
      return m_domain[i];
    };

    Index size()const {
      return m_domain.size();
    };

    void print(std::ostream &out) const {
      for(Index i = 0; i < size(); i++) {
        if(i == 0)
          out << m_domain[i].name;
        else
          out << ' ' << m_domain[i].name;
      }
    }

    void print_occ(std::ostream &out) const {
      if(valid_index(m_current_state))
        out << get_occ().name;
      else
        out << '?';
    }

    jsonParser &to_json(jsonParser &json) const;

  };


  /// overload for each template type to be used
  //    because we want to be able to do: void from_json(DoF *dof, const jsonParser &json)
  //

  // int version
  jsonParser &to_json(const OccupantDoF<int> &dof, jsonParser &json);

  void from_json(OccupantDoF<int> &dof, const jsonParser &json);

  // molecule version
  jsonParser &to_json(const OccupantDoF<Molecule> &dof, jsonParser &json);

  // Note: as a hack this expects dof.m_domain[0] to be present and have the right lattice!!!
  //   it's just used to set the lattice for all the Molecules
  void from_json(OccupantDoF<Molecule> &dof, const jsonParser &json);


  //****************************************************

  class ContinuousDoF : public DoF {
    double min_val, max_val, current_val;
    double current_min, current_max;

    /// Allows DoF to point to a remote value for faster/easier evaluation
    const double *m_remote_val;
  public:
    ContinuousDoF() : min_val(NAN), max_val(NAN), current_val(NAN),
      current_min(NAN), current_max(NAN), m_remote_val(NULL) {};

    ContinuousDoF(const std::string &tname, double tmin, double tmax) :
      DoF(tname), min_val(tmin), max_val(tmax), current_val(NAN),
      current_min(min_val), current_max(max_val), m_remote_val(NULL) {};

    ContinuousDoF(const std::string &tname, Index _ID, double tmin, double tmax) :
      DoF(tname, _ID), min_val(tmin), max_val(tmax), current_val(NAN),
      current_min(min_val), current_max(max_val), m_remote_val(NULL) {};

    double value() const {
      return current_val;
    };

    bool operator==(const ContinuousDoF &RHS) const {
      return((type_name() == RHS.type_name())
             && (ID() == RHS.ID())
             && almost_equal(value(), RHS.value()));
    };

    double remote_value() const {
      assert(m_remote_val && "In ContinuousDoF::remote_value() m_remote_val is NULL.\n");
      return *m_remote_val;
    };

    void register_remote(const double &remote_val) {
      m_remote_val = &remote_val;
    };

    void perturb_dof() {} //sets current_val to something between min_val and max_val

    //    ~ContinuousDoF(){}; //This has to be defined;

    ContinuousDoF *copy() const {
      return new ContinuousDoF(*this);
    };

    jsonParser &to_json(jsonParser &json) const {
      json.put_obj();
      json["DoF_type"] = "ContinuousDoF";
      json["m_type_name"] = m_type_name;
      json["min"] = min_val;
      json["max"] = max_val;
      json["current"] = current_val;
      return json;
    }

  };

  void from_json(ContinuousDoF &dof, const jsonParser &json);

  jsonParser &to_json(const ContinuousDoF &dof, jsonParser &json);

};
#endif
