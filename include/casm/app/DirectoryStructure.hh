#ifndef CASM_DirectoryStructure
#define CASM_DirectoryStructure

#include <string>
#include <vector>
#include <boost/filesystem.hpp>


namespace CASM {

  /// return path to current or parent directory containing ".casm" directory
  ///   if none found, return empty path
  inline fs::path find_casmroot(const fs::path &cwd) {
    fs::path dir(cwd);
    fs::path casmroot(".casm");

    while(!dir.empty()) {
      // if cwd contains ".casm", return cwd
      if(fs::is_directory(dir / casmroot))
        return dir;
      dir = dir.parent_path();
    }
    return dir;
  };

  /// \brief Specification of CASM project directory structure
  class DirectoryStructure {

  public:

    DirectoryStructure() {}

    DirectoryStructure(const fs::path _root) {
      _init(fs::absolute(_root));
    }


    // ** Query filesystem **

    /// \brief Check filesystem directory structure and return list of all basis set names
    std::vector<std::string> all_bset() const {
      return _all_settings("bset", m_root / m_bset_dir);
    }

    /// \brief Check filesystem directory structure and return list of all calctype names
    std::vector<std::string> all_calctype() const {
      return _all_settings("calctype", m_root / m_calc_dir / m_set_dir);
    }

    /// \brief Check filesystem directory structure and return list of all ref names for a given calctype
    std::vector<std::string> all_ref(std::string calctype) const {
      return _all_settings("ref", calc_settings_dir(calctype));
    }

    /// \brief Check filesystem directory structure and return list of all cluster expansion names
    std::vector<std::string> all_clex() const {
      return _all_settings("clex", m_root / m_clex_dir);
    }

    /// \brief Check filesystem directory structure and return list of all eci names
    std::vector<std::string> all_eci(std::string clex, std::string calctype, std::string ref, std::string bset) const {
      return _all_settings("eci", m_root / m_clex_dir / _clex(clex) / _calctype(calctype) / _ref(ref) / _bset(bset));
    }


    // ** File and Directory paths **


    // -- Project directory --------

    /// \brief Return casm project directory path
    fs::path root_dir() const {
      return m_root;
    }

    /// \brief Return prim.json path
    fs::path prim() const {
      return m_root / "prim.json";
    }

    /// \brief Return PRIM path
    fs::path PRIM() const {
      return m_root / "PRIM";
    }


    // -- Hidden .casm directory --------

    /// \brief Return hidden .casm dir path
    fs::path casm_dir() const {
      return m_root / m_casm_dir;
    }

    /// \brief Return project_settings.json path
    fs::path project_settings() const {
      return m_root / m_casm_dir / "project_settings.json";
    }

    /// \brief Return master scel_list.json path
    fs::path scel_list(std::string scelname) const {
      return m_root / m_casm_dir / "scel_list.json";
    }

    /// \brief Return master config_list.json file path
    fs::path config_list() const {
      return m_root / m_casm_dir / "config_list.json";
    }


    // -- Symmetry --------

    /// \brief Return symmetry directory path
    fs::path symmetry_dir() const {
      return m_root / m_sym_dir;
    }

    /// \brief Return lattice_point_group.json path
    fs::path lattice_point_group() const {
      return m_root / m_sym_dir / "lattice_point_group.json";
    }

    /// \brief Return factor_group.json path
    fs::path factor_group() const {
      return m_root / m_sym_dir / "factor_group.json";
    }

    /// \brief Return crystal_point_group.json path
    fs::path crystal_point_group() const {
      return m_root / m_sym_dir / "crystal_point_group.json";
    }


    // -- Basis sets --------

    /// \brief Return path to directory contain basis set info
    fs::path bset_dir(std::string bset) const {
      return m_root / m_bset_dir / _bset(bset);
    }

    /// \brief Return basis function specs (bspecs.json) file path
    fs::path bspecs(std::string bset) const {
      return bset_dir(bset) / "bspecs.json";
    }

    // \brief Returns path to the clust.json file
    fs::path clust(std::string bset) const {
      return bset_dir(bset) / "clust.json";
    }

    /// \brief Returns path to directory containing global clexulator
    fs::path clexulator_dir(std::string bset) const {
      return bset_dir(bset);
    }

    /// \brief Returns path to global clexulator source file
    fs::path clexulator_src(std::string project, std::string bset) const {
      return bset_dir(bset) / (project + "_Clexulator.cc");
    }

    /// \brief Returns path to global clexulator o file
    fs::path clexulator_o(std::string project, std::string bset) const {
      return bset_dir(bset) / (project + "_Clexulator.o");
    }
    /// \brief Returns path to global clexulator so file
    fs::path clexulator_so(std::string project, std::string bset) const {
      return bset_dir(bset) / (project + "_Clexulator.so");
    }

    /// \brief Returns path to eci.in, in bset directory
    fs::path eci_in(std::string bset) const {
      return bset_dir(bset) / "eci.in";
    }

    /// \brief Returns path to corr.in, in bset directory
    fs::path corr_in(std::string bset) const {
      return bset_dir(bset) / "corr.in";
    }

    /// \brief Returns path to prim_nlist.json, in bset directory
    fs::path prim_nlist(std::string bset) const {
      return bset_dir(bset) / "prim_nlist.json";
    }


    // -- Calculations and reference --------

    /// \brief Return supercell directory path (scelname has format SCELV_A_B_C_D_E_F)
    fs::path supercell_dir(std::string scelname) const {
      return m_root / m_calc_dir / scelname;
    }

    /// \brief Return configuration directory path (configname has format SCELV_A_B_C_D_E_F/I)
    fs::path configuration_dir(std::string configname) const {
      return m_root / m_calc_dir / configname;
    }


    /// \brief Return calculation settings directory path, for global settings
    fs::path calc_settings_dir(std::string calctype) const {
      return m_root / m_calc_dir / m_set_dir / _calctype(calctype);
    }

    /// \brief Return calculation settings directory path, for supercell specific settings
    fs::path supercell_calc_settings_dir(std::string scelname, std::string calctype) const {
      return supercell_dir(scelname) / m_set_dir / _calctype(calctype);
    }

    /// \brief Return calculation settings directory path, for configuration specific settings
    fs::path configuration_calc_settings_dir(std::string configname, std::string calctype) const {
      return configuration_dir(configname) / m_set_dir / _calctype(calctype);
    }

    /// \brief Return calculated properties file path
    fs::path calculated_properties(std::string configname, std::string calctype) const {
      return configuration_dir(configname) / _calctype(calctype) / "properties.calc.json";
    }


    /// \brief Return calculation reference settings directory path, for global settings
    fs::path ref_dir(std::string calctype, std::string ref) const {
      return calc_settings_dir(calctype) / _ref(ref);
    }

    /// \brief Return calculation reference settings directory path, for supercell settings
    fs::path supercell_ref_dir(std::string scelname, std::string calctype, std::string ref) const {
      return supercell_calc_settings_dir(scelname, calctype) / _ref(ref);
    }

    /// \brief Return calculation reference settings directory path, for configuration settings
    fs::path configuration_ref_dir(std::string configname, std::string calctype, std::string ref) const {
      return configuration_calc_settings_dir(configname, calctype) / _ref(ref);
    }

    /// \brief Return composition axes file path
    fs::path composition_axes(std::string calctype, std::string ref) const {
      return ref_dir(calctype, ref) / "composition_axes.json";
    }

    /// \brief Return reference state file path, for global settings
    fs::path ref_state(std::string calctype, std::string ref, int index) const {
      return ref_dir(calctype, ref) / _ref_state(index);
    }

    /// \brief Return reference state file path, for supercell specific settings
    fs::path supercell_ref_state(std::string scelname, std::string calctype, std::string ref, int index) const {
      return supercell_ref_dir(scelname, calctype, ref) / _ref_state(index);
    }

    /// \brief Return reference state file path, for configuration specific settings
    fs::path configuration_ref_state(std::string configname, std::string calctype, std::string ref, int index) const {
      return configuration_ref_dir(configname, calctype, ref) / _ref_state(index);
    }


    // -- Cluster expansions --------

    /// \brief Returns path to eci directory
    fs::path clex_dir(std::string clex) const {
      return m_root / m_clex_dir / _clex(clex);
    }

    /// \brief Returns path to eci directory
    fs::path eci_dir(std::string clex, std::string calctype, std::string ref, std::string bset, std::string eci) const {
      return clex_dir(clex) / _calctype(calctype) / _ref(ref) / _bset(bset) / _eci(eci);
    }

    /// \brief Returns path to eci.out
    fs::path eci_out(std::string clex, std::string calctype, std::string ref, std::string bset, std::string eci) const {
      return eci_dir(clex, calctype, ref, bset, eci) / "eci.out";
    }

    /// \brief Returns path to eci.in, in eci fitting directory
    fs::path energy(std::string clex, std::string calctype, std::string ref, std::string bset, std::string eci) const {
      return eci_dir(clex, calctype, ref, bset, eci) / "energy";
    }

    /// \brief Returns path to eci.in, in eci fitting directory
    fs::path eci_in(std::string clex, std::string calctype, std::string ref, std::string bset, std::string eci) const {
      return eci_dir(clex, calctype, ref, bset, eci) / "eci.in";
    }

    /// \brief Returns path to corr.in, in eci fitting directory
    fs::path corr_in(std::string clex, std::string calctype, std::string ref, std::string bset, std::string eci) const {
      return eci_dir(clex, calctype, ref, bset, eci) / "corr.in";
    }


    // -- other maybe temporary --------------------------

    /// \brief Return cluster specs (CSPECS) file path
    fs::path CSPECS(std::string bset) const {
      return bset_dir(bset) / "CSPECS";
    }

    // \brief Returns path to the clust.json file
    fs::path FCLUST(std::string bset) const {
      return bset_dir(bset) / "FCLUST.json";
    }



  private:

    std::string _bset(std::string bset) const {
      return std::string("bset.") + bset;
    }

    std::string _calctype(std::string calctype) const {
      return std::string("calctype.") + calctype;
    }

    std::string _ref(std::string ref) const {
      return std::string("ref.") + ref;
    }

    std::string _clex(std::string clex) const {
      return std::string("clex.") + clex;
    }

    std::string _eci(std::string eci) const {
      return std::string("eci.") + eci;
    }

    std::string _ref_state(int index) const {
      return std::string("properties.ref_state.") + std::to_string(index) + ".json";
    }


    void _init(const fs::path &_root) {
      m_root = _root;
      m_casm_dir = ".casm";
      m_bset_dir = "basis_sets";
      m_calc_dir = "training_data";
      m_set_dir = "settings";
      m_sym_dir = "symmetry";
      m_clex_dir = "cluster_expansions";
    }

    /// \brief Find all directories at 'location' that match 'pattern.something'
    ///        and return a std::vector of the 'something'
    std::vector<std::string> _all_settings(std::string pattern, fs::path location) const {

      std::vector<std::string> all;
      std::string dir;
      pattern += ".";

      // get all
      if(!fs::exists(location)) {
        return all;
      }
      fs::directory_iterator it(location);
      fs::directory_iterator end_it;
      for(; it != end_it; ++it) {
        if(fs::is_directory(*it)) {
          dir = it->path().filename().string();
          if(dir.substr(0, pattern.size()) == pattern) {
            all.push_back(dir.substr(pattern.size(), dir.size()));
          }
        }
      }

      std::sort(all.begin(), all.end());

      return all;
    }

    fs::path m_root;
    std::string m_casm_dir;
    std::string m_bset_dir;
    std::string m_calc_dir;
    std::string m_set_dir;
    std::string m_sym_dir;
    std::string m_clex_dir;

  };
}

#endif
