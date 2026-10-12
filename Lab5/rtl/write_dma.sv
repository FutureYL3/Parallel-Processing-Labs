module write_dma (
    input logic clk, rst_n, start,
    input logic [31:0] command_address, command_stride, command_m, command_n,
    output logic busy, done,
    output logic aw_valid,
    input logic aw_ready,
    output logic [31:0] aw_address, aw_beats,
    output logic w_valid,
    input logic w_ready,
    output logic [63:0] w_data,
    output logic [7:0] w_strobe,
    output logic w_last,
    input logic b_valid,
    output logic b_ready,
    output logic [31:0] spm_row, spm_column,
    input logic [63:0] spm_data
);
    typedef enum logic [1:0] {IDLE, ADDRESS, DATA, RESPONSE} state_t;
    state_t state;
    logic [31:0] address_q, stride_q, m_q, n_q, row_q, beat_q;
    assign busy = state != IDLE;
    assign aw_valid = state == ADDRESS;
    assign aw_address = address_q+row_q*stride_q;
    assign aw_beats = (n_q+1)/2;
    assign w_valid = state == DATA;
    assign w_data = spm_data;
    assign w_strobe = beat_q*2+1<n_q ? 8'hff : 8'h0f;
    assign w_last = (beat_q+1)*2 >= n_q;
    assign b_ready = state == RESPONSE;
    assign spm_row = row_q;
    assign spm_column = beat_q*2;
    always_ff @(posedge clk) begin
        if (!rst_n) begin
            state <= IDLE; done <= 0;
            address_q <= 0; stride_q <= 0; m_q <= 0; n_q <= 0; row_q <= 0; beat_q <= 0;
        end else begin
            done <= 0;
            case (state)
                IDLE: if (start) begin
                    address_q <= command_address; stride_q <= command_stride;
                    m_q <= command_m; n_q <= command_n; row_q <= 0; beat_q <= 0; state <= ADDRESS;
                end
                ADDRESS: if (aw_ready) begin beat_q <= 0; state <= DATA; end
                DATA: if (w_ready) begin
                    if (w_last) state <= RESPONSE;
                    else beat_q <= beat_q+1;
                end
                RESPONSE: if (b_valid) begin
                    if (row_q+1<m_q) begin row_q <= row_q+1; state <= ADDRESS; end
                    else begin state <= IDLE; done <= 1; end
                end
                default: state <= IDLE;
            endcase
        end
    end
endmodule
