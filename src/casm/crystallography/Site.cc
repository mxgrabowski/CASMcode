#include "casm/crystallography/Site.hh"

#include "casm/basis_set/FunctionVisitor.hh"

namespace CASM {


  Site::Site(const Lattice &init_home) :
    Coordinate(init_home), m_nlist_ind(-1), m_type_ID(-1), m_site_occupant("occupation", Array<Molecule>()) {
    //site_occupant.set_value(0);
  }

  //****************************************************

  Site::Site(const Coordinate &init_pos, const std::string &occ_name) :
    Coordinate(init_pos), m_nlist_ind(-1), m_type_ID(-1), m_site_occupant("occupation", Array<Molecule>()) {

    Molecule tMol(*home);
    tMol.name = occ_name;
    tMol.push_back(AtomPosition(0, 0, 0, occ_name, *home));

    Array<Molecule> tocc;
    tocc.push_back(tMol);
    m_site_occupant.set_domain(tocc);
    m_site_occupant.set_value(0);

    return;
  }

  //****************************************************

  bool Site::is_vacant()const {
    return site_occupant().is_specified()
           && site_occupant().get_occ().is_vacancy();
  }

  //****************************************************

  Index Site::nlist_ind() const {
    return m_nlist_ind;
  };

  //****************************************************

  std::string Site::occ_name() const {
    if(!site_occupant().is_specified())
      return "?";
    else
      return site_occupant().get_occ().name;
  }

  //****************************************************

  const Molecule &Site::occ() const {
    return site_occupant().get_occ();
  };

  //****************************************************
  Lattice const *Site::home_ptr() const {
    return home;
  };

  //****************************************************
  /**
   *
   */
  //****************************************************

  Site &Site::apply_sym(const SymOp &op) {
    Coordinate::apply_sym(op);
    for(Index i = 0; i < site_occupant().size(); i++) {
      m_site_occupant[i].apply_sym_no_trans(op);
    }

    return *this;
  }

  //****************************************************
  /**
   *
   */
  //****************************************************

  Site &Site::apply_sym_no_trans(const SymOp &op) {
    Coordinate::apply_sym_no_trans(op);
    for(Index i = 0; i < site_occupant().size(); i++) {
      m_site_occupant[i].apply_sym_no_trans(op);
    }

    return *this;
  }

  //****************************************************
  /**
   *
   */
  //****************************************************

  void Site::set_SD_flag(bool sdx, bool sdy, bool sdz) {
    set_SD_flag(Vector3<bool>(sdx, sdy, sdz));
    return;
  }

  //****************************************************
  /**
   *
   */
  //****************************************************
  void Site::set_SD_flag(const Vector3<bool> &SDvec) {
    SD_flag = SDvec;
    Index nm, na;
    for(nm = 0; nm < site_occupant().size(); nm++) {
      for(na = 0; na < site_occupant()[nm].size(); na++) {
        m_site_occupant[nm][na].SD_flag = SD_flag;
      }
    }
    return;
  }



  //****************************************************
  /**
   *
   */
  //****************************************************

  Site &Site::operator+=(const Coordinate &translation) {
    Coordinate::operator += (translation);
    return *this;
  }

  //****************************************************
  /**
   *
   */
  //****************************************************

  Site &Site::operator-=(const Coordinate &translation) {
    Coordinate::operator -= (translation);
    return *this;
  }

  //*******************************************************************************************
  /**
   *
   */
  //*******************************************************************************************

  bool Site::compare(const Coordinate &test_coord, double compare_tol) const {
    return (min_dist(test_coord) < compare_tol);
  }

  //*******************************************************************************************
  /**
   *
   */
  //*******************************************************************************************

  bool Site::compare(const Site &test_site, double compare_tol) const {
    return (compare_type(test_site) && min_dist(test_site) < compare_tol);
  }

  //*******************************************************************************************
  /**
   *
   *
   */
  //*******************************************************************************************

  bool Site::compare(const Site &test_site, const Coordinate &shift, double compare_tol) const {

    return (compare_type(test_site)) && (min_dist(test_site + shift) < compare_tol);

  }

  //*******************************************************************************************
  /**
   *
   */
  //*******************************************************************************************

  bool Site::compare_type(const Site &test_site) const {
    assert(((site_occupant().size() <= 1 || test_site.site_occupant().size() <= 1)
            || ((site_occupant().is_specified() && test_site.site_occupant().is_specified())
                || (!site_occupant().is_specified() && !test_site.site_occupant().is_specified())))
           && "In Site::compare_type() comparing initialized occupant to uninitialized occupant!  This isn't a good idea!");

    return (_type_ID() == test_site._type_ID()) && site_occupant().value() == test_site.site_occupant().value();
  }

  //*******************************************************************************************

  bool Site::operator==(const Site &test_site) const {
    return (compare_type(test_site) && Coordinate::operator==(test_site));
  }

  //****************************************************

  bool Site::contains(const std::string &name) const {
    for(Index i = 0; i < site_occupant().size(); i++)
      if(site_occupant()[i].contains(name)) {
        return true;
      }
    return false;
  }

  //****************************************************

  bool Site::contains(const std::string &name, int &index) const {
    for(Index i = 0; i < site_occupant().size(); i++)
      if(site_occupant()[i].contains(name)) {
        index = i;
        return true;
      }
    return false;
  }

  //****************************************************

  void Site::invalidate(COORD_TYPE mode) {
    Coordinate::invalidate(mode);
    for(Index i = 0; i < site_occupant().size(); i++) {
      m_site_occupant[i].invalidate(mode);
    }
    return;
  }

  //****************************************************

  void Site::set_lattice(const Lattice &new_lat) {
    Coordinate::set_lattice(new_lat);
    for(Index i = 0; i < site_occupant().size(); i++) {
      m_site_occupant[i].set_lattice(new_lat);
    }
    return;
  }

  //****************************************************
  //John G. This is to use with set_lattice from Coordinate
  void Site::set_lattice(const Lattice &new_lat, COORD_TYPE mode) {
    Coordinate::set_lattice(new_lat, mode);
    for(Index i = 0; i < site_occupant().size(); i++) {
      m_site_occupant[i].set_lattice(new_lat);
    }
    return;
  }

  //****************************************************

  Array<std::string> Site::allowed_occupants() const {
    Array<std::string> occ_list;
    for(Index i = 0; i < site_occupant().size(); i++) {
      occ_list.push_back(site_occupant()[i].name);
    }
    return occ_list;
  }

  //****************************************************

  void Site::set_basis_ind(Index new_ind) {
    Coordinate::set_basis_ind(new_ind);
    if(valid_index(new_ind))
      m_occupant_basis.accept(OccFuncBasisIndexer(new_ind));
  }

  //****************************************************

  void Site::set_nlist_ind(Index new_ind) {
    if(new_ind == m_nlist_ind)
      return;

    m_site_occupant.set_ID(new_ind);
    m_occupant_basis.update_dof_IDs(Array<Index>(1, m_nlist_ind), Array<Index>(1, new_ind));
    //for(Index i = 0; i < displacement.size(); i++) {
    //displacement[i].set_ID(new_ind);
    //}
    m_nlist_ind = new_ind;
    return;
  }

  //****************************************************
  //   read site, including all possible occupants
  void Site::read(std::istream &stream, bool SD_is_on) {
    char ch;

    Molecule tMol(*home);

    Coordinate::read(stream);
    if(SD_is_on) {
      for(int i = 0; i < 3; i++) {
        stream >> ch;
        if(ch == 'T') {
          SD_flag[i] = true;
        }
        else if(ch == 'F') {
          SD_flag[i] = false;
        }
      }
    }

    Array<Molecule> tocc;

    ch = stream.peek();
    //    while(ch != '\n' && !stream.eof()) {
    while(ch != '\n' && ch != ':' && !stream.eof()) {
      //      while((ch < 'a' || ch > 'z') && (ch < 'A' || ch > 'Z') && ch != '\n' && !stream.eof()) {
      while((ch < 'a' || ch > 'z') && (ch < 'A' || ch > 'Z') && ch != '\n' && ch != ':' && !stream.eof()) {
        stream.ignore();
        ch = stream.peek();
      }
      if(ch != '\n' && ch != ':' && !stream.eof()) {
        //Need to change this part for real molecules
        tMol.clear();
        stream >> tMol.name;


        tMol.push_back(AtomPosition(0, 0, 0, tMol.name, *home, SD_flag));
        tocc.push_back(tMol);
      }
      ch = stream.peek();
    }

    if(ch == ':') {
      stream.ignore();
      stream.ignore();

      tMol.clear();
      stream >> tMol.name;
      tMol.push_back(AtomPosition(0, 0, 0, tMol.name, *home, SD_flag));

      if(tocc.size()) {
        m_site_occupant.set_domain(tocc);
        Index index = tocc.size();
        for(Index i = 0; i < tocc.size(); i++)
          if(tocc[i].name == tMol.name) {
            index = i;
            break;
          }
        if(index == tocc.size()) {
          std::cerr << "ERROR in Site::read(). Occupying molecule not listed in possible occupants" << std::endl;
          std::cout << "  occupying molecule name: " << tMol.name << "  index: " << index << std::endl;
          std::cout << "  possible occupants: ";
          for(Index i = 0; i < tocc.size(); i++)
            std::cout << tocc[i].name << " ";
          std::cout << " " << std::endl;
          exit(1);
        }
        else
          m_site_occupant.set_value(index);

      }
      else {
        std::cerr << "WARNING: Trying to read Site info, but no valid input was received." << std::endl;
      }
      m_type_ID = -1;
      return;
    }

    if(tocc.size()) {
      m_site_occupant.set_domain(tocc);
    }
    else {
      std::cerr << "WARNING: Trying to read Site info, but no valid input was received." << std::endl;
    }
    stream.ignore(1000, '\n');

    m_type_ID = -1;
    return;
  }

  //****************************************************
  // read site, using 'elem' as site occupant domain
  void Site::read(std::istream &stream, std::string &elem, bool SD_is_on) {
    char ch;

    Molecule tMol(*home);

    Coordinate::read(stream);
    if(SD_is_on) {
      for(int i = 0; i < 3; i++) {
        stream >> ch;
        if(ch == 'T') {
          SD_flag[i] = true;
        }
        else if(ch == 'F') {
          SD_flag[i] = false;
        }
      }
    }

    Array<Molecule> tocc;

    tMol.clear();
    tMol.name = elem;
    tMol.push_back(AtomPosition(0, 0, 0, tMol.name, *home, SD_flag));
    tocc.push_back(tMol);

    if(tocc.size()) {
      m_site_occupant.set_domain(tocc);
    }
    else {
      std::cerr << "WARNING: Trying to read Site info, but no valid input was received." << std::endl;
    }
    stream.ignore(1000, '\n');

    m_type_ID = -1;
    return;
  }


  //****************************************************
  /**	Print coordinate of site with name of all possible
  *		occupying molecule
   */
  //****************************************************

  void Site::print(std::ostream &stream) const {
    //site_occupant().get_occ().print(stream, *this, SD_is_on);
    Coordinate::print(stream);
    stream << " ";
    site_occupant().print(stream);
    stream << std::flush;

    return;
  }

  //****************************************************
  /**	Print coordinate of site with name of occupying molecule
   */
  //****************************************************

  void Site::print_occ(std::ostream &stream) const {
    //site_occupant().get_occ().print(stream, *this, SD_is_on);
    Site::print(stream);
    stream << " :: ";
    site_occupant().print_occ(stream);
    stream << std::flush;

    return;
  }

  //****************************************************
  /**	Print each AtomPosition in current molecule,
   *		with name of occupying atom
   */
  //****************************************************

  void Site::print_mol(std::ostream &stream, int spaces, char delim, bool SD_is_on) const {
    site_occupant().get_occ().print(stream, *this, spaces, delim, SD_is_on);
    return;
  }

  //****************************************************

  std::ostream &operator<< (std::ostream &stream, const Site &site) {
    site.print(stream);
    return stream;
  }

  //****************************************************
  /**
   * Fill the occupation basis set of the site with a
   * specified type of basis function (spin, occupation,
   * chebychev etc)
   */
  //****************************************************

  void Site::fill_occupant_basis(const char &basis_type) {
    Array<double> site_conc(site_occupant().size(), 0.0);
    switch(basis_type) {
    case 'c':
      site_conc.resize(site_occupant().size(), 1.0 / double(site_occupant().size()));
      m_occupant_basis.construct_orthonormal_discrete_functions(site_occupant(), site_conc, basis_ind());
      break;
    case 'o':
      if(site_conc.size())
        site_conc[0] = 1.0;
      m_occupant_basis.construct_orthonormal_discrete_functions(site_occupant(), site_conc, basis_ind());
      break;
    default:
      std::cerr << "ERROR in Site::fill_occupant_basis" << std::endl;
      std::cerr << "The specified type of basis functions does not exist. You picked " << basis_type << " ,but at the moment there's just chevychev and occupation." << std::endl;
      break;
    }
    return;
  }
  //\John G 011013

  ///Copy another basis set into m_occupant_basis
  void Site::update_data_members(const Site &_ref_site) {
    //    std::cout<<"Updating data members"<<std::endl;
    m_occupant_basis = _ref_site.occupant_basis();
    m_site_occupant = _ref_site.site_occupant();
    //displacement = _ref_site.displacement;

    m_site_occupant.set_ID(m_nlist_ind);
    m_occupant_basis.update_dof_IDs(Array<Index>(1, _ref_site.nlist_ind()), Array<Index>(1, m_nlist_ind));
    //for(Index i = 0; i < displacement.size(); i++) {
    //displacement[i].set_ID(m_nlist_ind);
    //}
    m_type_ID = -1;
    return;
  }

  //****************************************************

  jsonParser &Site::to_json(jsonParser &json) const {
    json.put_obj();

    // class Site : public Coordinate
    const Coordinate &coord = *this;
    json["coordinate"].put(coord);

    // MoleculeOccupant site_occupant;
    json["site_occupant"] = site_occupant();

    // BasisSet occupant_basis;
    json["occupant_basis"] = m_occupant_basis;

    // Vector3<bool> SD_flag;
    json["SD_flag"] = SD_flag;

    // int m_nlist_ind;
    json["m_nlist_ind"] = m_nlist_ind;


    return json;
  }

  //****************************************************

  void Site::from_json(const jsonParser &json) {
    try {

      // class Site : public Coordinate
      Coordinate &coord = *this;
      //std::cout<<"Trying to read in the Site"<<std::endl;
      //std::cout<<"Reading in the Coordinate"<<std::endl;
      CASM::from_json(coord, json["coordinate"]);

      Array<Molecule> temp_mol_array(1, Molecule(*home));
      m_site_occupant.set_domain(temp_mol_array);
      //std::cout<<"Reading in site_occupant"<<std::endl;
      // MoleculeOccupant site_occupant;
      CASM::from_json(m_site_occupant, json["site_occupant"]);

      // BasisSet occupant_basis; (no reading BasisSet right now)
      // CASM::from_json(m_occupant_basis, json["occupant_basis"]);

      //std::cout<<"Reading in SD_flag"<<std::endl;
      // Vector3<bool> SD_flag;
      CASM::from_json(SD_flag, json["SD_flag"]);

      //std::cout<<"Reading in m_list_ind"<<std::endl;
      // int m_nlist_ind;
      CASM::from_json(m_nlist_ind, json["m_nlist_ind"]);

      //std::cout<<"Finished Site::from_json"<<std::endl;

    }
    catch(...) {
      /// re-throw exceptions
      throw;
    }
    m_type_ID = -1;
  };

  jsonParser &to_json(const Site &site, jsonParser &json) {
    return site.to_json(json);
  }

  void from_json(Site &site, const jsonParser &json) {
    site.from_json(json);
  }

  //*******************************************************************************************

  bool Site::_compare_type_no_ID(const Site &test_site) const {
    //compare domain but not value
    return site_occupant().compare(test_site.site_occupant(), false);
  }

  //*******************************************************************************************

  Index Site::_type_ID() const {
    if(!valid_index(m_type_ID)) {
      for(m_type_ID = 0; m_type_ID < _type_prototypes().size(); m_type_ID++) {
        if(_compare_type_no_ID(_type_prototypes()[m_type_ID]))
          return m_type_ID;
      }
      //std::cout << "NEW TYPE PROTOTYPE!\n";
      //print_occ(std::cout);
      //std::cout << " : TYPE_ID-> " << m_type_ID << "\n";
      _type_prototypes().push_back(*this);
    }
    return m_type_ID;
  }

  //****************************************************

  Site operator*(const SymOp &LHS, const Site &RHS) {
    return Site(RHS).apply_sym(LHS);
  }

  //****************************************************

  Site operator+(const Site &LHS, const Coordinate &RHS) {
    return Site(LHS) += RHS;
  }

  //****************************************************

  Site operator+(const Coordinate &LHS, const Site &RHS) {
    return Site(RHS) += LHS;
  }



};
