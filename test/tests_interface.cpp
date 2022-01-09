/**
 *  \file tests/tests_interface.cpp
 *
 *  \brief  
 *
 *  \author  Olivia Quinet
 */

#include <catch2/catch.hpp>
#include "../include/cppsas7bdat/sas7bdat.hpp"
#include "../include/cppsas7bdat/datasource_ifstream.hpp"
#include "../include/cppsas7bdat/datasink_null.hpp"
#include "data.hpp"

namespace {
  template<typename _DataSink>
  auto get_reader(const char* _pcszfilename, _DataSink&& _datasink) {
    return cppsas7bdat::Reader(cppsas7bdat::datasource::ifstream(_pcszfilename), std::forward<_DataSink>(_datasink));
  }  
  auto get_reader(const char* _pcszfilename) {
    return get_reader(_pcszfilename, cppsas7bdat::datasink::null());
    //return cppsas7bdat::Reader(cppsas7bdat::datasource::ifstream(_pcszfilename), cppsas7bdat::datasink::null());
  }  
}

SCENARIO("When I try to read a non existing file with the public interface, an exception is thrown", "[interface][not_a_valid_file]")
{
  GIVEN("An invalid path") {
    THEN("An exception is thrown") {
      REQUIRE_THROWS_WITH(get_reader(invalid_path), "not_a_valid_file");
    }
  }
}


SCENARIO("When I try to read a file too short with the public interface, an exception is thrown", "[interface][file too short]")
{
  GIVEN("A path to a too short file") {
    THEN("an exception is thrown") {
      REQUIRE_THROWS_WITH(get_reader(file_too_short), "header_too_short");
    }
  }
}

SCENARIO("When I try to read a file with an invalid magic number with the public interface, an exception is thrown", "[interface][header_too_short]")
{
  GIVEN("A path to a file with an invalid magic number") {
    THEN("an exception is thrown") {
      REQUIRE_THROWS_WITH(get_reader(invalid_magic_number), "invalid_magic_number");
    }
  }
}

SCENARIO("When I try to read a valid file with the public interface, no exception is thrown", "[interface][valid_file]")
{
  GIVEN("A path to a valid file") {
    THEN("No exception is thrown") {
      REQUIRE_NOTHROW(get_reader(file1));
    }
  }
}

#include <charconv>
#include <type_traits>

namespace {
  struct MyTestDataSink {
    COLUMNS columns;
    using iterator = decltype(std::declval<const json>()[""].items().begin());
    iterator ref_data_iter;
    iterator ref_data_iter_end;
    size_t ref_irow{0};
    size_t row_read{0};
    size_t ncols{0};
    
    template<typename _iter>
    MyTestDataSink(_iter&& _ref_data_iter, _iter&& _ref_data_iter_end)
      : ref_data_iter(std::forward<_iter>(_ref_data_iter)),
	ref_data_iter_end(std::forward<_iter>(_ref_data_iter_end))
    {
      get_ref_irow();
    }

    void get_ref_irow() {
      const std::string line = ref_data_iter.key();
      std::from_chars(line.data(), line.data()+line.size(), ref_irow);
    };
    
    void set_properties([[maybe_unused]]const Properties& _properties) {
      columns = COLUMNS(_properties.metadata.columns);
      ncols = columns.size();
    }
      
    void read_row([[maybe_unused]]const size_t _irow,
		  [[maybe_unused]]Column::PBUF _p) {
      if(row_read == ref_irow) {
	const auto values = ref_data_iter.value();
	for(size_t icol=0; icol < ncols; ++icol) {
	  const auto& column = columns[icol];
	  const auto refval = values[icol];
	  INFO("Colname=" << column.name << '[' << icol << "] row=" << ref_irow);
	  switch(column.type()) {
	  case cppsas7bdat::Column::Type::string:
	    CHECK(column.get_string(_p) == refval);
	    break;
	  case cppsas7bdat::Column::Type::integer:
	    CHECK(column.get_integer(_p) == refval);
	    break;
	  case cppsas7bdat::Column::Type::number: {
	    auto x = column.get_number(_p);
	    if(std::isnan(x)) {
	      CHECK(refval.is_null());
	    } else {
	      CHECK(x == refval);
	    }
	  }	break;
	  case cppsas7bdat::Column::Type::datetime:
	    CHECK(column.get_datetime(_p) == get_datetime(refval));
	    break;
	  case cppsas7bdat::Column::Type::date:
	    CHECK(column.get_date(_p) == get_date(refval));
	    break;
	  case cppsas7bdat::Column::Type::time:
	    CHECK(column.get_time(_p) == get_time(refval));
	    break;
	  default:
	    CHECK(false);
	  }
	}
	++ref_data_iter;
	if(ref_data_iter != ref_data_iter_end) get_ref_irow();
      }
      ++row_read;
    }
  };
}

SCENARIO("When I read a file with the public interface, the data are read properly", "[inteface][read_data]")
{
  const auto data = GENERATE(from_range(files().j.items().begin(),files().j.items().end()));
  
  const std::string filename = data.key();
  const auto ref_header = data.value()["Header"];
  const auto ref_columns = data.value()["Columns"];
  auto ref_data = data.value()["Data"].items();
  
  GIVEN(fmt::format("A file {},", filename)) {
    // Skip big5 files
    if(filename.find("big5") != filename.npos) return;
    WHEN("The data is read") {
      auto reader = get_reader(filename.c_str(), MyTestDataSink(ref_data.begin(), ref_data.end()));
      const auto& columns = reader.properties().metadata.columns;
      THEN("The data values are correct") {
	reader.read_all();
      }
    }
  }
}