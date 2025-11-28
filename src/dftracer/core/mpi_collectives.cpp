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

// Point-to-point wrappers (timing only)
extern "C" int MPI_Send(const void *buf, int count, MPI_Datatype datatype,
                         int dest, int tag, MPI_Comm comm) {
  MPIScope scope("MPI_Send", get_logger());
  return PMPI_Send(buf, count, datatype, dest, tag, comm);
}

extern "C" int MPI_Isend(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm,
                          MPI_Request *request) {
  MPIScope scope("MPI_Isend", get_logger());
  return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

extern "C" int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm) {
  MPIScope scope("MPI_Ssend", get_logger());
  return PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}

extern "C" int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm) {
  MPIScope scope("MPI_Rsend", get_logger());
  return PMPI_Rsend(buf, count, datatype, dest, tag, comm);
}

extern "C" int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm) {
  MPIScope scope("MPI_Bsend", get_logger());
  return PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}

extern "C" int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src,
                         int tag, MPI_Comm comm, MPI_Status *status) {
  MPIScope scope("MPI_Recv", get_logger());
  return PMPI_Recv(buf, count, datatype, src, tag, comm, status);
}

extern "C" int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src,
                          int tag, MPI_Comm comm, MPI_Request *request) {
  MPIScope scope("MPI_Irecv", get_logger());
  return PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
}

extern "C" int MPI_Sendrecv(const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, int dest, int sendtag,
                             void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, int source, int recvtag,
                             MPI_Comm comm, MPI_Status *status) {
  MPIScope scope("MPI_Sendrecv", get_logger());
  return PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf,
                       recvcount, recvtype, source, recvtag, comm, status);
}

extern "C" int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  MPIScope scope("MPI_Wait", get_logger());
  return PMPI_Wait(request, status);
}

extern "C" int MPI_Waitall(int count, MPI_Request array_of_requests[],
                            MPI_Status array_of_statuses[]) {
  MPIScope scope("MPI_Waitall", get_logger());
  return PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

extern "C" int MPI_Waitany(int count, MPI_Request array_of_requests[],
                            int *index, MPI_Status *status) {
  MPIScope scope("MPI_Waitany", get_logger());
  return PMPI_Waitany(count, array_of_requests, index, status);
}

extern "C" int MPI_Waitsome(int incount, MPI_Request array_of_requests[],
                             int *outcount, int array_of_indices[],
                             MPI_Status array_of_statuses[]) {
  MPIScope scope("MPI_Waitsome", get_logger());
  return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices,
                       array_of_statuses);
}
