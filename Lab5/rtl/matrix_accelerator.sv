module matrix_accelerator #(
    parameter int unsigned ARRAY = 8, MAX_TILE = 64, MAX_K = 128
) (
    input logic clk, rst_n, start,
    input logic [31:0] cfg_m, cfg_k, cfg_n, cfg_tile_m, cfg_tile_k, cfg_tile_n,
    input logic [31:0] cfg_a_base, cfg_b_base, cfg_c_base,
    input logic cfg_double_buffer,
    output logic busy, done, compute_active,
    output logic [63:0] cycles, compute_cycles, input_wait_cycles, output_cycles, overlap_cycles, mac_count,
    output logic ar_valid,
    input logic ar_ready,
    output logic [31:0] ar_address, ar_beats,
    input logic r_valid,
    output logic r_ready,
    input logic [63:0] r_data,
    output logic aw_valid,
    input logic aw_ready,
    output logic [31:0] aw_address, aw_beats,
    output logic w_valid,
    input logic w_ready,
    output logic [63:0] w_data,
    output logic [7:0] w_strobe,
    output logic w_last,
    input logic b_valid,
    output logic b_ready
);
    typedef enum logic [3:0] {IDLE, TILE_INIT, ZERO_C, WAIT_INPUT, CLEAR_PE, RUN_PE, STORE_C, WRITE_START, WAIT_OUTPUT} state_t;
    state_t state;
    logic [31:0] m_q, k_q, n_q, tm_q, tk_q, tn_q, a_base_q, b_base_q, c_base_q;
    logic double_q, current_buffer;
    logic [1:0] buffer_ready;
    logic [31:0] m_offset, n_offset, k_offset, actual_m, actual_n, actual_k;
    logic [31:0] zero_row, zero_column, sub_m, sub_n, step_q, store_row;
    logic load_start, load_buffer, load_busy, load_done;
    logic [31:0] load_a, load_b, load_k;
    logic spm_write, spm_matrix, spm_buffer;
    logic [31:0] spm_row, spm_column, spm_lanes;
    logic [63:0] spm_data;
    logic write_start, write_busy, write_done;
    logic [31:0] c_read_row, c_read_column;
    logic [63:0] c_read_data;
    logic pe_clear, c_write, c_zero;
    logic [31:0] c_row, c_column, c_lanes, macs_this_cycle;
    logic signed [15:0] left_a [ARRAY], top_b [ARRAY];
    logic [ARRAY-1:0] left_valid, top_valid;
    logic signed [31:0] pe_sums [ARRAY][ARRAY], c_values [ARRAY];

    assign actual_m = m_q-m_offset < tm_q ? m_q-m_offset : tm_q;
    assign actual_n = n_q-n_offset < tn_q ? n_q-n_offset : tn_q;
    assign actual_k = k_q-k_offset < tk_q ? k_q-k_offset : tk_q;
    assign compute_active = state == RUN_PE;
    assign pe_clear = state == CLEAR_PE;
    assign write_start = state == WRITE_START;
    assign c_write = state == ZERO_C || state == STORE_C;
    assign c_zero = state == ZERO_C;
    assign c_row = c_zero ? zero_row : sub_m+store_row;
    assign c_column = c_zero ? zero_column : sub_n;
    assign c_lanes = actual_n-c_column < ARRAY ? actual_n-c_column : ARRAY;
    always_comb
        for (int c = 0; c < ARRAY; ++c) c_values[c] = pe_sums[store_row][c];

    read_dma loader (
        .clk, .rst_n, .start(load_start), .command_buffer(load_buffer),
        .a_address(load_a), .b_address(load_b), .a_stride(k_q*2), .b_stride(n_q*2),
        .command_m(actual_m), .command_n(actual_n), .command_k(load_k),
        .busy(load_busy), .done(load_done), .ar_valid, .ar_ready, .ar_address, .ar_beats, .r_valid, .r_ready, .r_data,
        .spm_write, .spm_matrix, .spm_buffer, .spm_row, .spm_column, .spm_lanes, .spm_data
    );
    write_dma writer (
        .clk, .rst_n, .start(write_start), .command_address(c_base_q+(m_offset*n_q+n_offset)*4),
        .command_stride(n_q*4), .command_m(actual_m), .command_n(actual_n),
        .busy(write_busy), .done(write_done), .aw_valid, .aw_ready, .aw_address, .aw_beats,
        .w_valid, .w_ready, .w_data, .w_strobe, .w_last, .b_valid, .b_ready,
        .spm_row(c_read_row), .spm_column(c_read_column), .spm_data(c_read_data)
    );
    scratchpad #(.ARRAY(ARRAY), .MAX_TILE(MAX_TILE), .MAX_K(MAX_K)) spm (
        .clk, .input_write(spm_write), .input_matrix(spm_matrix), .input_buffer(spm_buffer),
        .input_row(spm_row), .input_column(spm_column), .input_lanes(spm_lanes), .input_data(spm_data),
        .compute_active, .compute_buffer(current_buffer), .step(step_q), .sub_m, .sub_n,
        .tile_m(actual_m), .tile_n(actual_n), .tile_k(actual_k), .left_a, .top_b, .left_valid, .top_valid,
        .c_write, .c_zero, .c_row, .c_column, .c_lanes, .c_values,
        .read_row(c_read_row), .read_column(c_read_column), .read_data(c_read_data)
    );
    systolic_array #(.ARRAY(ARRAY)) array_core (
        .clk, .rst_n, .clear(pe_clear), .enable(compute_active),
        .left_a, .top_b, .left_valid, .top_valid, .sums(pe_sums), .macs_this_cycle
    );

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE; busy <= 0; done <= 0;
            m_q <= 0; k_q <= 0; n_q <= 0; tm_q <= 0; tk_q <= 0; tn_q <= 0;
            a_base_q <= 0; b_base_q <= 0; c_base_q <= 0; double_q <= 0;
            m_offset <= 0; n_offset <= 0; k_offset <= 0; current_buffer <= 0; buffer_ready <= 0;
            zero_row <= 0; zero_column <= 0; sub_m <= 0; sub_n <= 0; step_q <= 0; store_row <= 0;
            load_start <= 0; load_buffer <= 0; load_a <= 0; load_b <= 0; load_k <= 0;
            cycles <= 0; compute_cycles <= 0; input_wait_cycles <= 0;
            output_cycles <= 0; overlap_cycles <= 0; mac_count <= 0;
        end else begin
            load_start <= 0;
            if (load_done) buffer_ready[spm_buffer] <= 1'b1;
            if (busy) cycles <= cycles+64'd1;
            if (compute_active) begin
                compute_cycles <= compute_cycles+64'd1;
                mac_count <= mac_count+64'(macs_this_cycle);
                if (load_busy) overlap_cycles <= overlap_cycles+64'd1;
            end
            if (state == WAIT_INPUT && !buffer_ready[current_buffer]) input_wait_cycles <= input_wait_cycles+64'd1;
            if (write_busy) output_cycles <= output_cycles+64'd1;
            case (state)
                IDLE: if (start) begin
                    m_q <= cfg_m; k_q <= cfg_k; n_q <= cfg_n;
                    tm_q <= cfg_tile_m; tk_q <= cfg_tile_k; tn_q <= cfg_tile_n;
                    a_base_q <= cfg_a_base; b_base_q <= cfg_b_base; c_base_q <= cfg_c_base;
                    double_q <= cfg_double_buffer;
                    m_offset <= 0; n_offset <= 0; done <= 0; busy <= 1;
                    cycles <= 0; compute_cycles <= 0; input_wait_cycles <= 0;
                    output_cycles <= 0; overlap_cycles <= 0; mac_count <= 0;
                    state <= TILE_INIT;
                end
                TILE_INIT: begin
                    current_buffer <= 0; buffer_ready <= 0; k_offset <= 0;
                    zero_row <= 0; zero_column <= 0; sub_m <= 0; sub_n <= 0;
                    load_buffer <= 0; load_a <= a_base_q+m_offset*k_q*2;
                    load_b <= b_base_q+n_offset*2; load_k <= k_q<tk_q ? k_q : tk_q;
                    load_start <= 1; state <= ZERO_C;
                end
                ZERO_C: begin
                    if (zero_column+ARRAY<actual_n) zero_column <= zero_column+ARRAY;
                    else begin
                        zero_column <= 0;
                        if (zero_row+1<actual_m) zero_row <= zero_row+1;
                        else state <= WAIT_INPUT;
                    end
                end
                WAIT_INPUT: if (buffer_ready[current_buffer]) begin
                    sub_m <= 0; sub_n <= 0; state <= CLEAR_PE;
                    // Prefetch the next K panel into the other physical SPM buffer.
                    if (double_q && k_offset+tk_q<k_q) begin
                        load_buffer <= !current_buffer;
                        buffer_ready[!current_buffer] <= 0;
                        load_a <= a_base_q+(m_offset*k_q+k_offset+tk_q)*2;
                        load_b <= b_base_q+((k_offset+tk_q)*n_q+n_offset)*2;
                        load_k <= k_q-k_offset-tk_q<tk_q ? k_q-k_offset-tk_q : tk_q;
                        load_start <= 1;
                    end
                end
                CLEAR_PE: begin step_q <= 0; store_row <= 0; state <= RUN_PE; end
                RUN_PE: begin
                    if (step_q+1>=actual_k+2*ARRAY-2) begin store_row <= 0; state <= STORE_C; end
                    else step_q <= step_q+1;
                end
                STORE_C: begin
                    if (store_row+1<ARRAY && sub_m+store_row+1<actual_m) store_row <= store_row+1;
                    else if (sub_n+ARRAY<actual_n) begin sub_n <= sub_n+ARRAY; state <= CLEAR_PE; end
                    else if (sub_m+ARRAY<actual_m) begin sub_m <= sub_m+ARRAY; sub_n <= 0; state <= CLEAR_PE; end
                    else begin
                        buffer_ready[current_buffer] <= 0;
                        if (k_offset+tk_q<k_q) begin
                            k_offset <= k_offset+tk_q; state <= WAIT_INPUT;
                            if (double_q) current_buffer <= !current_buffer;
                            else begin
                                load_buffer <= current_buffer;
                                load_a <= a_base_q+(m_offset*k_q+k_offset+tk_q)*2;
                                load_b <= b_base_q+((k_offset+tk_q)*n_q+n_offset)*2;
                                load_k <= k_q-k_offset-tk_q<tk_q ? k_q-k_offset-tk_q : tk_q;
                                load_start <= 1;
                            end
                        end else state <= WRITE_START;
                    end
                end
                WRITE_START: state <= WAIT_OUTPUT;
                WAIT_OUTPUT: if (write_done) begin
                    if (n_offset+tn_q<n_q) begin n_offset <= n_offset+tn_q; state <= TILE_INIT; end
                    else if (m_offset+tm_q<m_q) begin m_offset <= m_offset+tm_q; n_offset <= 0; state <= TILE_INIT; end
                    else begin state <= IDLE; busy <= 0; done <= 1; end
                end
                default: state <= IDLE;
            endcase
        end
    end
endmodule
