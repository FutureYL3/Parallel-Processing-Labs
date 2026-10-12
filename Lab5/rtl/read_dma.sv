module read_dma (
    input logic clk, rst_n, start, command_buffer,
    input logic [31:0] a_address, b_address, a_stride, b_stride, command_m, command_n, command_k,
    output logic busy, done,
    output logic ar_valid,
    input logic ar_ready,
    output logic [31:0] ar_address, ar_beats,
    input logic r_valid,
    output logic r_ready,
    input logic [63:0] r_data,
    output logic spm_write, spm_matrix, spm_buffer,
    output logic [31:0] spm_row, spm_column, spm_lanes,
    output logic [63:0] spm_data
);
    typedef enum logic [1:0] {IDLE, ADDRESS, DATA} state_t;
    state_t state;
    logic matrix_q, buffer_q;
    logic [31:0] a_q, b_q, a_stride_q, b_stride_q, m_q, n_q, k_q, row_q, beat_q;
    logic [31:0] width, height;
    assign width = matrix_q ? n_q : k_q;
    assign height = matrix_q ? k_q : m_q;
    assign busy = state != IDLE;
    assign ar_valid = state == ADDRESS;
    assign ar_address = matrix_q ? b_q+row_q*b_stride_q : a_q+row_q*a_stride_q;
    assign ar_beats = (width+3)/4;
    assign r_ready = state == DATA;
    assign spm_write = r_valid && r_ready;
    assign spm_matrix = matrix_q;
    assign spm_buffer = buffer_q;
    assign spm_row = row_q;
    assign spm_column = beat_q*4;
    assign spm_lanes = width-beat_q*4 < 4 ? width-beat_q*4 : 4;
    assign spm_data = r_data;
    always_ff @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE; done <= 0; matrix_q <= 0; buffer_q <= 0;
            a_q <= 0; b_q <= 0; a_stride_q <= 0; b_stride_q <= 0;
            m_q <= 0; n_q <= 0; k_q <= 0; row_q <= 0; beat_q <= 0;
        end else begin
            done <= 0;
            case (state)
                IDLE: if (start) begin
                    a_q <= a_address; b_q <= b_address;
                    a_stride_q <= a_stride; b_stride_q <= b_stride;
                    m_q <= command_m; n_q <= command_n; k_q <= command_k;
                    buffer_q <= command_buffer; matrix_q <= 0; row_q <= 0; beat_q <= 0;
                    state <= ADDRESS;
                end
                ADDRESS: if (ar_ready) begin state <= DATA; beat_q <= 0; end
                DATA: if (r_valid) begin
                    if ((beat_q+1)*4 >= width) begin
                        beat_q <= 0;
                        if (row_q+1 < height) begin row_q <= row_q+1; state <= ADDRESS; end
                        else if (!matrix_q) begin matrix_q <= 1; row_q <= 0; state <= ADDRESS; end
                        else begin state <= IDLE; done <= 1; end
                    end else beat_q <= beat_q+1;
                end
                default: state <= IDLE;
            endcase
        end
    end
endmodule
