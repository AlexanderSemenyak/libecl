#include <util.h>
#include <block_fs.h>
#include <vector.h>
#include <signal.h>
#include <msg.h>


void install_SIGNALS(void) {
  signal(SIGSEGV , util_abort_signal);    /* Segmentation violation, i.e. overwriting memory ... */
  signal(SIGINT  , util_abort_signal);    /* Control C */
  signal(SIGTERM , util_abort_signal);    /* If killing the enkf program with SIGTERM (the default kill signal) you will get a backtrace. Killing with SIGKILL (-9) will not give a backtrace.*/
}


int main(int argc , char ** argv) {
  install_SIGNALS();
  const char * mount_file      = argv[1];
  if (block_fs_is_mount(mount_file)) {
    block_fs_sort_type sort_mode = OFFSET_SORT;
    const char * pattern         = NULL;
    int iarg;

    for (iarg = 2; iarg < argc; iarg++) {
      if (argv[iarg][0] == '-') {
        /** OK - this is an option .. */
      }
      else pattern = argv[iarg];
    }
    
    {
      block_fs_type * block_fs = block_fs_mount(mount_file , 1 , 0 , 1 , 0 , false , false );
      vector_type   * files    = block_fs_alloc_filelist( block_fs , pattern , sort_mode , false );
      {
        int i;
        msg_type * msg = msg_alloc("Deleting file: ");
        msg_show( msg );
        //for (i=0; i < vector_get_size( files ); i++) {
        //  const file_node_type * node = vector_iget_const( files , i );
        //  printf("%-40s   %10d %ld    \n",file_node_get_filename( node ), file_node_get_data_size( node ) , file_node_get_node_offset( node ));
        //}

        for (i=0; i < vector_get_size( files ); i++) {
          const file_node_type * node = vector_iget_const( files , i );
          msg_update( msg , file_node_get_filename( node ) );
          block_fs_unlink_file( block_fs , file_node_get_filename( node ));
        }
        msg_free( msg , true );
        printf("Final fragmentation: %5.2f \n", block_fs_get_fragmentation( block_fs ));
      }
      vector_free( files );
      block_fs_close( block_fs , false );
    }
  } else 
    fprintf(stderr,"The file:%s does not seem to be a block_fs mount file.\n" , mount_file);
}