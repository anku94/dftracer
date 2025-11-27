#include <mpi.h>

#include <memory>

#include <dftracer/core/common/constants.h>
#include <dftracer/core/common/datastructure.h>
#include <dftracer/core/common/typedef.h>
#include <dftracer/core/df_logger.h>

namespace {
constexpr ConstEventNameType CATEGORY = "MPI";

DFTLogger *get_logger() {
  static std::shared_ptr<DFTLogger> logger = DFT_LOGGER_INIT();
  return logger.get();
}

class MPIScope {
 public:
  MPIScope(const char *event_name, DFTLogger *logger)
      : name_(event_name), logger_(logger), metadata_(nullptr), start_(0) {
    if (logger_ == nullptr) return;
    if (logger_->include_metadata) {
      metadata_ = new dftracer::Metadata();
    }
    logger_->enter_event();
    start_ = logger_->get_time();
  }

  ~MPIScope() {
    if (logger_ == nullptr) return;
    TimeResolution end = logger_->get_time();
    logger_->log(name_, CATEGORY, start_, end - start_, metadata_);
    logger_->exit_event();
  }

 private:
  const char *name_;
  DFTLogger *logger_;
  dftracer::Metadata *metadata_;
  TimeResolution start_;
};
}  // namespace

extern "C" int MPI_Barrier(MPI_Comm comm) {
  MPIScope scope("MPI_Barrier", get_logger());
  return PMPI_Barrier(comm);
}

extern "C" int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype,
                           int root, MPI_Comm comm) {
  MPIScope scope("MPI_Bcast", get_logger());
  return PMPI_Bcast(buffer, count, datatype, root, comm);
}

extern "C" int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, int root,
                            MPI_Comm comm) {
  MPIScope scope("MPI_Reduce", get_logger());
  return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

extern "C" int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op,
                               MPI_Comm comm) {
  MPIScope scope("MPI_Allreduce", get_logger());
  return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

extern "C" int MPI_Gather(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                            MPI_Datatype recvtype, int root, MPI_Comm comm) {
  MPIScope scope("MPI_Gather", get_logger());
  return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                     recvtype, root, comm);
}

extern "C" int MPI_Scatter(const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, int root, MPI_Comm comm) {
  MPIScope scope("MPI_Scatter", get_logger());
  return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                      recvtype, root, comm);
}

extern "C" int MPI_Allgather(const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm) {
  MPIScope scope("MPI_Allgather", get_logger());
  return PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                        recvtype, comm);
}

extern "C" int MPI_Alltoall(const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm) {
  MPIScope scope("MPI_Alltoall", get_logger());
  return PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                       recvtype, comm);
}

extern "C" int MPI_Scan(const void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  MPIScope scope("MPI_Scan", get_logger());
  return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

extern "C" int MPI_Exscan(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op,
                            MPI_Comm comm) {
  MPIScope scope("MPI_Exscan", get_logger());
  return PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}
